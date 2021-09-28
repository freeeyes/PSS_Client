#pragma once

//tcp¿Í»§¶Ë
//add by freeeyes

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <chrono>
#include <asio.hpp>
#include "pss_common.h"
#include "tms.hpp"
#include "IOClient.h"

using namespace std::chrono;
using asio::ip::tcp;
using std::placeholders::_1;

class CASIOClient : public std::enable_shared_from_this<CASIOClient>, public IIOClient
{
public:
    explicit CASIOClient(asio::io_context* io_context, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose);

    int get_tms_logic_id();

    bool start(int connect_id, const std::string& server_ip, short server_port) final;

    void do_read();

    void do_write_format_data(short command_id, const char* data, size_t length) final;

    void do_write_immediately(const char* data, size_t length) final;

    void close_socket() final;

    bool get_connect_state();

    void connect_handler(const asio::error_code& ec);

    void set_write_time();

    int get_time_pass_seconds() final;

    void do_check_timeout(int seconds) final;

    void reconnect() final;

private:
    tcp::socket socket_;
    system_clock::time_point recv_last_timer_ = system_clock::now();
    system_clock::time_point write_last_timer_ = system_clock::now();
    char recv_buffer[recv_buffer_max_size_];
    int connect_id_ = 0;
    bool is_connect_ = false;
    std::shared_ptr<ipacket_format> packet_format_;
    std::shared_ptr<ipacket_dispose> packet_dispose_;
    std::string server_ip_;
    short server_port_;
    std::mutex thread_mutex_;
};

