#pragma once

#define WIN32_LEAN_AND_MEAN 

#include <iostream>
#include <memory>
#include <chrono>
#include "pss_common.h"

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef PSSTCPCLIENT_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C" _declspec(dllimport)
#endif
#else
#define DECLDIR extern "C"
#endif

//链接类型
enum class em_io_type
{
    IO_TYPE_TCP = 0,
    IO_TYPE_UDP,
    IO_TYPE_TTY,
};

DECLDIR int load_module(int thread_count = 1, int timer_check_seconds = 30);
DECLDIR void unload_module();
DECLDIR int start_client(const std::string& server_ip, short server_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose, em_io_type io_type = em_io_type::IO_TYPE_TCP);
DECLDIR bool reconnect_server(int client_id);
DECLDIR bool client_send_data(int client_id, const std::string& send_buff, int send_size);
DECLDIR bool client_send_format_data(int client_id, short command_id, const std::string& send_buff, int send_size);
DECLDIR void close_client(int client_id);
DECLDIR int add_timer(int work_thread_id, std::chrono::milliseconds time_interval_milliseconds, task_function func);
DECLDIR int add_timer_loop(int work_thread_id, std::chrono::seconds begin_delay_milliseconds, std::chrono::milliseconds time_interval_milliseconds, task_function func);
DECLDIR bool close_timer_id(int timer_id);
