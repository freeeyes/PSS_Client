#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <asio.hpp>
#include "pss_common.h"

using asio::ip::tcp;
using std::placeholders::_1;


const int recv_buffer_max_size_ = 10240;

class CASIOClient : public std::enable_shared_from_this<CASIOClient>
{
public:
    explicit CASIOClient(asio::io_context* io_context, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr);

    bool start(int connect_id, const std::string& server_ip, short server_port);

    void do_read();

    void do_write_immediately(const char* data, size_t length);

    void close_socket();

    bool get_connect_state();

    void connect_handler(const asio::error_code& ec);

    client_connect_ptr get_client_connect_ptr();

    client_dis_connect_ptr get_client_dis_connect_ptr();

    client_recv_ptr get_client_recv_ptr();

private:
    tcp::socket socket_;
    char recv_buffer[recv_buffer_max_size_];
    int connect_id_ = 0;
    bool is_connect_ = false;
    client_connect_ptr connect_ptr_ = nullptr;
    client_dis_connect_ptr dis_connect_ptr_ = nullptr;
    client_recv_ptr recv_ptr_ = nullptr;
};

