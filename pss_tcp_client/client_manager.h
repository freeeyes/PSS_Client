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
        //��ʱ��������еĿͻ����Ƿ���
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
        //����µ�ID
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
                //����ҵ��ˣ����Զ�����
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

        //�رն�ʱ��
        App_tms::instance()->Close();

        //�ر����е�IO
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
        //�󶨶�ʱ��
        //��ʼ���¼��̶߳���
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
            //���IO���������ﲻ��ʹ�ö��IO�̣߳�����Ҫ�ˡ�
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
            //����IO�߳�
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

        //����һ�ᣬ�����߳�������ϡ�
        this_thread::sleep_for(chrono::milliseconds(5));
    }

private:
    map<int, std::shared_ptr<CASIOClient>> asio_client_list_;
    vector<CIO_Context_info*> io_context_list_;
    int io_context_list_count_ = 0;
    std::thread tt_context_;
    int curr_client_id_ = 1;
    int timer_check_seconds_ = 30000; //��ʱ�����ʱ��
    asio::io_context  io_context_;
    bool thread_is_run_ = false;
};

using App_Client_Manager = PSS_singleton<CClient_Manager>;

