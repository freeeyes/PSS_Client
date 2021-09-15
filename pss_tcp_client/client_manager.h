#pragma once

#include "singleton.h"
#include "pss_common.h"
#include "ASIOClient.h"
#include "tms.hpp"
#include <vector>

class CIO_Context_info
{
public:
    CIO_Context_info() = default;

    asio::io_context* io_context_ = nullptr;
    bool thread_is_run_ = false;
};

class CClient_Manager
{
public:
    CClient_Manager()
    {
        CIO_Context_info* io_context_info = new CIO_Context_info();
        io_context_info->io_context_ = new asio::io_context();
        io_context_list_.emplace_back(io_context_info);
        io_context_list_count_ = 0;

        App_tms::instance()->Init();
    }

    void timer_check()
    {
        //定时器检查所有的客户端是否到期
        std::cout << "[CClient_Manager::timer_check]begin" << std::endl;
        for (const auto& io_session : asio_client_list_)
        {
            int time_pass_seconds = io_session.second->get_time_pass_seconds();
            if (time_pass_seconds > timer_check_seconds_)
            {
                //std::cout << "[CClient_Manager::timer_check]connect_id=" << io_session.first << " is timeout " << time_pass_seconds << "." << std::endl;
                io_session.second->do_check_timeout(time_pass_seconds);
            }
        }
        std::cout << "[CClient_Manager::timer_check]end" << std::endl;
    }

    int create_new_client(std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    {
        //获得新的ID
        int client_id = curr_client_id_;
        curr_client_id_++;

        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            return -1;
        }
        else
        {
            asio_client_list_[client_id] = std::make_shared<CASIOClient>(&io_context_,
                packet_format,
                packet_dispose);
            return client_id;
        }
    }

    bool start_client(int client_id, const std::string& server_ip, short server_port)
    {
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            return f->second->start(client_id, server_ip, server_port);
        }
        else
        {
            return false;
        }
    }

    bool client_send_data(int client_id, const std::string& send_buff, int send_size)
    {
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            f->second->do_write_immediately(send_buff.c_str(), send_size);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool close_client(int client_id)
    {
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            f->second->close_socket();
            asio_client_list_.erase(client_id);
            return true;
        }
        else
        {
            return false;
        }
    }

    void close()
    {
        for (const auto& client : asio_client_list_)
        {
            client.second->close_socket();
        }

        asio_client_list_.clear();

        //关闭定时器
        App_tms::instance()->Close();

        //关闭所有的IO
        io_context_.stop();
        tt_context_.join();

        for (int i = 0; i < io_context_list_count_; i++)
        {
            delete io_context_list_[i];
        }

        io_context_list_.clear();
        thread_is_run_ = false;
        io_context_list_count_ = 0;
    }

    void run(int io_context_count = 1, int timer_check_seconds = 30)
    {
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
            for (int i = 0; i < add_io_context_count; i++)
            {
                CIO_Context_info* io_context_info = new CIO_Context_info();
                //io_context_info->io_context_ = new asio::io_context();
                io_context_info->io_context_ = &io_context_;
                io_context_list_.emplace_back(io_context_info);
            }

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
                        std::cout << "[CClient_Manager::run](io_context) is run." << std::endl;
                        thread_is_run_ = true;
                        while (true)
                        {
                            io_context_.run();
                        }
                        std::cout << "[CClient_Manager::run](io_context) is over." << std::endl;
                    }
                    catch (asio::error_code ec)
                    {
                        std::cout << "[CClient_Manager::run](io_context) is error=" << ec.message() << std::endl;
                    }
                });
        }

        //挂起一会，等子线程启动完毕。
        this_thread::sleep_for(chrono::milliseconds(5));
    }

private:
    map<int, std::shared_ptr<CASIOClient>> asio_client_list_;
    vector<CIO_Context_info*> io_context_list_;
    int io_context_list_count_ = 0;
    std::thread tt_context_;
    int curr_client_id_ = 1;
    int timer_check_seconds_ = 30000; //定时器检测时间
    asio::io_context  io_context_;
    bool thread_is_run_ = false;
};

using App_Client_Manager = PSS_singleton<CClient_Manager>;

