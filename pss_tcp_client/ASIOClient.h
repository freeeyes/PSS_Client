#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <chrono>
#include <asio.hpp>
#include "pss_common.h"
#include "tms.hpp"

using namespace std::chrono;
using asio::ip::tcp;
using std::placeholders::_1;

//Ä¬ÈÏµÄÃüÁîid
const short connect_command_id = 0x0001;
const short disconnect_command_id = 0x0002;
const short time_check_command_id = 0x0003;

const int recv_buffer_max_size_ = 10240;

class CASIOClient : public std::enable_shared_from_this<CASIOClient>
{
public:
    explicit CASIOClient(asio::io_context* io_context, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose);

    int get_tms_logic_id();

    bool start(int connect_id, const std::string& server_ip, short server_port);

    void do_read();

    void do_write_immediately(const char* data, size_t length);

    void close_socket();

    bool get_connect_state();

    void connect_handler(const asio::error_code& ec);

    void set_write_time();

    int get_time_pass_seconds();

    void do_check_timeout(int seconds);

private:
    tcp::socket socket_;
    system_clock::time_point recv_last_timer_ = system_clock::now();
    system_clock::time_point write_last_timer_ = system_clock::now();
    char recv_buffer[recv_buffer_max_size_];
    int connect_id_ = 0;
    bool is_connect_ = false;
    std::shared_ptr<ipacket_format> packet_format_;
    std::shared_ptr<ipacket_dispose> packet_dispose_;
};

