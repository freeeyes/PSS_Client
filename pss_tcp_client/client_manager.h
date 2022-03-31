#pragma once

#include "singleton.h"
#include "pss_common.h"
#include "ASIOClient.h"
#include "ASIOUDPClient.h"
#include "ASIOTTYClient.h"
#include <vector>

//记录客户端超时的数据结构
class CClient_Timeout_Info
{
public:
    std::shared_ptr<IIOClient> io_client_ = nullptr;
    int timeout_soecnds = 0;
};

class CClient_Manager
{
public:
    CClient_Manager()
    {
        io_context_list_count_ = 0;

        App_tms::instance()->Init();
    }

    int get_tms_logic_id(int connect_id)
    {
        int logic_thread_count = App_tms::instance()->Get_Logic_Count();
        return connect_id % logic_thread_count;
    }

    void timer_check()
    {
        //考虑到使用锁的问题，在这里直接使用线程消息去处理

        App_tms::instance()->AddMessage(0, [this]() {
            thread_mutex_.lock();
            //定时器检查所有的客户端是否到期
            vector<CClient_Timeout_Info> timeout_session_list;
            for (const auto& io_session : asio_client_list_)
            {
                int time_pass_seconds = io_session.second->get_time_pass_seconds();
                if (time_pass_seconds > timer_check_seconds_)
                {
                    CClient_Timeout_Info timeout_info_;
                    timeout_info_.io_client_ = io_session.second;
                    timeout_info_.timeout_soecnds = time_pass_seconds;
                    timeout_session_list.emplace_back(timeout_info_);
                }
            }
            thread_mutex_.unlock();

            //调用超时事件
            for (const auto& timeout_info_ : timeout_session_list)
            {
                timeout_info_.io_client_->do_check_timeout(timeout_info_.timeout_soecnds);
            }
            });
    }

    int start_client_tcp(const std::string& server_ip, short server_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);

        //这里获得当前的ID，需要加锁
        int client_id = curr_client_id_;
        curr_client_id_++;

        //丢到消息线程里去做
        asio_client_list_[client_id] = std::make_shared<CASIOClient>(&io_context_,
            packet_format,
            packet_dispose);

        asio_client_list_[client_id]->start(client_id, server_ip, server_port);

        return client_id;
    }

    int start_client_udp(const std::string& server_ip, short server_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);

        //这里获得当前的ID，需要加锁
        int client_id = curr_client_id_;
        curr_client_id_++;

        //丢到消息线程里去做
        asio_client_list_[client_id] = std::make_shared<CASIOUDPClient>(&io_context_,
            packet_format,
            packet_dispose);

        asio_client_list_[client_id]->start(client_id, server_ip, server_port);

        return client_id;
    }

    int start_client_tty(const std::string& tty_name, short tty_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);

        //这里获得当前的ID，需要加锁
        int client_id = curr_client_id_;
        curr_client_id_++;

        //丢到消息线程里去做
        asio_client_list_[client_id] = std::make_shared<CASIOTTYClient>(&io_context_,
            packet_format,
            packet_dispose);

        asio_client_list_[client_id]->start(client_id, tty_name, tty_port);

        return client_id;
    }

    bool client_send_data(int client_id, const std::string& send_buff, int send_size)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            auto client = f->second;
            client->do_write_immediately(send_buff.c_str(), send_size);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool client_send_format_data(int client_id, short command_id, const std::string& send_buff, int send_size)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            auto client = f->second;
            client->do_write_format_data(command_id, send_buff.c_str(), send_size);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool close_client(int client_id)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            f->second->close_client_socket();
            asio_client_list_.erase(f);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool reconnect_server(int client_id)
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            f->second->reconnect();
            return true;
        }
        else
        {
            //std::cout << "[reconnect_server]no find connect_id(" << client_id << ")." << std::endl;
            return false;
        }
    }

    void close()
    {
        std::lock_guard<std::mutex> guard(thread_mutex_);

        //关闭所有的IO
        if (thread_is_run_ == true)
        {
            for (const auto& client : asio_client_list_)
            {
                client.second->close_client_socket();
            }

            asio_client_list_.clear();

            //等待异步处理链接断开消息
            this_thread::sleep_for(std::chrono::milliseconds(100));

            io_context_.stop();
            tt_context_.join();

            //关闭定时器
            App_tms::instance()->Close();
        }

        thread_is_run_ = false;
        io_context_list_count_ = 0;
    }

    void run(int io_context_count = 1, int timer_check_seconds = 30)
    {
        if (true == is_Init_)
        {
            return;
        }

        //绑定定时器
        //初始化事件线程队列
        for (int i = 0; i < io_context_count; i++)
        {
            App_tms::instance()->CreateLogic(i);
        }

        timer_check_seconds_ = timer_check_seconds;

        auto timer_check = std::bind(&CClient_Manager::timer_check, this);
        App_tms::instance()->AddMessage_loop(0, 
            std::chrono::seconds(0), 
            std::chrono::seconds(timer_check_seconds_), 
            timer_check);

        int add_io_context_count = io_context_count - io_context_list_count_;
        if (add_io_context_count > 0)
        {
            //添加IO操作，这里不在使用多个IO线程，不需要了。
            io_context_list_count_ += add_io_context_count;
        }

        if (false == thread_is_run_)
        {
            //启动IO线程
            tt_context_ = std::thread([this]()
                {
                    asio::executor_work_guard<asio::io_context::executor_type> work_guard_
                        = asio::make_work_guard(io_context_);
                    try
                    {
                        //std::cout << "[CClient_Manager::run](io_context) is run." << std::endl;
                        thread_is_run_ = true;
                        while (true)
                        {
                            io_context_.run();
                        }
                        //std::cout << "[CClient_Manager::run](io_context) is over." << std::endl;
                    }
                    catch (asio::error_code ec)
                    {
                        std::cout << "[CClient_Manager::run](io_context) is error=" << ec.message() << std::endl;
                    }
                });
        }

        //挂起一会，等子线程启动完毕。
        this_thread::sleep_for(chrono::milliseconds(10));
        is_Init_ = true;
    }

private:
    map<int, std::shared_ptr<IIOClient>> asio_client_list_;
    int io_context_list_count_ = 0;
    std::thread tt_context_;
    int curr_client_id_ = 1;
    int timer_check_seconds_ = 30000; //定时器检测时间
    asio::io_context  io_context_;
    bool thread_is_run_ = false;
    std::mutex thread_mutex_;
    bool is_Init_ = false;
};

using App_Client_Manager = PSS_singleton<CClient_Manager>;

