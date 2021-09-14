#pragma once

#include "singleton.h"
#include "pss_common.h"
#include "ASIOClient.h"
#include "io_timer.h"
#include <vector>

class CIO_Context_info
{
public:
    CIO_Context_info() = default;

    asio::io_context* io_context_ = nullptr;
    std::thread tt_context_;
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
        io_context_list_count_ = 1;
    }

    void timer_check()
    {
        //定时器检查所有的客户端是否到期
        std::cout << "[CClient_Manager::timer_check]begin" << std::endl;
        for (const auto& io_session : asio_client_list_)
        {
            int time_pass_seconds = io_session.second->get_time_pass_seconds();
            if (time_pass_seconds > timer_check_minseconds_ / 1000)
            {
                //std::cout << "[CClient_Manager::timer_check]connect_id=" << io_session.first << " is timeout " << time_pass_seconds << "." << std::endl;
                io_session.second->do_check_timeout(time_pass_seconds);
            }
        }
        std::cout << "[CClient_Manager::timer_check]end" << std::endl;
    }

    int create_new_client(int client_id, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr, time_check_ptr time_ptr)
    {
        auto io_context_index = client_id % io_context_list_count_;
        auto io_context_ = io_context_list_[io_context_index]->io_context_;

        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            return -1;
        }
        else
        {
            asio_client_list_[client_id] = std::make_shared<CASIOClient>(io_context_,
                connect_ptr,
                dis_connect_ptr,
                recv_ptr,
                time_ptr);
            return client_id;
        }
    }

    int create_new_client(client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr, time_check_ptr time_ptr)
    {
        //获得新的ID
        int client_id = curr_client_id_;
        curr_client_id_++;

        auto io_context_index = client_id % io_context_list_count_;
        auto io_context_ = io_context_list_[io_context_index]->io_context_;

        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            return -1;
        }
        else
        {
            asio_client_list_[client_id] = std::make_shared<CASIOClient>(io_context_,
                connect_ptr,
                dis_connect_ptr,
                recv_ptr,
                time_ptr);
            return client_id;
        }
    }

    bool start_client(int client_id, const std::string& server_ip, short server_port)
    {
        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            if(f->second->get_connect_state() == true)
            {
                //如果找到了，则自动重连
                client_connect_ptr _client_connect_ptr = f->second->get_client_connect_ptr();
                client_dis_connect_ptr _client_dis_connect_ptr = f->second->get_client_dis_connect_ptr();
                client_recv_ptr _client_recv_ptr = f->second->get_client_recv_ptr();
                time_check_ptr _client_time_ptr = f->second->get_time_check_ptr();

                create_new_client(client_id, _client_connect_ptr, _client_dis_connect_ptr, _client_recv_ptr, _client_time_ptr);
                return f->second->start(client_id, server_ip, server_port);
            }
            else
            {
                return f->second->start(client_id, server_ip, server_port);
            }
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

        //关闭所有的线程
        for (int i = 0; i < io_context_list_count_; i++)
        {
            io_context_list_[i]->io_context_->stop();
            io_context_list_[i]->tt_context_.join();

            delete io_context_list_[i]->io_context_;
            delete io_context_list_[i];
        }

        io_context_list_.clear();
        io_context_list_count_ = 0;
    }

    void run(int io_context_count = 1, int timer_check_seconds = 30)
    {
        //定时器
        timer_check_minseconds_ = timer_check_seconds * 1000;

        auto timer_check = std::bind(&CClient_Manager::timer_check, this);
        asio_client_timer_.StartTimer(timer_check_minseconds_, timer_check);

        int add_io_context_count = io_context_count - io_context_list_count_;
        if (add_io_context_count > 0)
        {
            //添加线程
            for (int i = 0; i < add_io_context_count; i++)
            {
                CIO_Context_info* io_context_info = new CIO_Context_info();
                io_context_info->io_context_ = new asio::io_context();
                io_context_list_.emplace_back(io_context_info);
            }

            io_context_list_count_ += add_io_context_count;
        }

        for (int i = 0; i < io_context_list_count_; i++)
        {
            if (io_context_list_[i]->thread_is_run_ == false)
            {
                io_context_list_[i]->tt_context_ = std::thread([this, i]()
                    {
                        asio::executor_work_guard<asio::io_context::executor_type> work_guard_ 
                            = asio::make_work_guard(*io_context_list_[i]->io_context_);
                        try
                        {
                            std::cout << "[CClient_Manager::run](" << i << ") is run." << std::endl;
                            io_context_list_[i]->thread_is_run_ = true;
                            while (true)
                            {
                                io_context_list_[i]->io_context_->run();
                            }
                            std::cout << "[CClient_Manager::run](" << i << ") is over." << std::endl;
                        }
                        catch (asio::error_code ec)
                        {
                            std::cout << "[CClient_Manager::run](" << i << ") is error=" << ec.message() << std::endl;
                        }

                        

                    });
            }
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
    int timer_check_minseconds_ = 30000; //定时器检测时间

    io_timer asio_client_timer_;

};

using App_Client_Manager = PSS_singleton<CClient_Manager>;

