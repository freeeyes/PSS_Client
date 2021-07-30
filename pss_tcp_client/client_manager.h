#pragma once

#include "singleton.h"
#include "pss_common.h"
#include "ASIOClient.h"
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

    bool create_new_client(int client_id, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr)
    {
        auto io_context_index = client_id % io_context_list_count_;
        auto io_context_ = io_context_list_[io_context_index]->io_context_;

        auto f = asio_client_list_.find(client_id);
        if (f != asio_client_list_.end())
        {
            return false;
        }
        else
        {
            asio_client_list_[client_id] = std::make_shared<CASIOClient>(io_context_,
                connect_ptr,
                dis_connect_ptr,
                recv_ptr);
            return true;
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

    void run(int io_context_count = 1)
    {
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
                        io_context_list_[i]->io_context_->run();
                        io_context_list_[i]->thread_is_run_ = true;
                    });
            }
        }

        //挂起一会，等子线程启动完毕。
        this_thread::sleep_for(chrono::microseconds(5));
    }

private:
    map<int, std::shared_ptr<CASIOClient>> asio_client_list_;
    vector<CIO_Context_info*> io_context_list_;
    int io_context_list_count_ = 0;
    std::thread tt_context_;
};

using App_Client_Manager = PSS_singleton<CClient_Manager>;

