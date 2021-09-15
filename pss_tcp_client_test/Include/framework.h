#pragma once

#define WIN32_LEAN_AND_MEAN 

#include <iostream>
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

//数据解析组包类(需要上层逻辑提供实现类)
class ipacket_format
{
public:
    virtual bool format_recv_buffer(int connect_id, const char* recv_buffer, size_t buffer_length) = 0;
    virtual std::string format_send_buffer(int connect_id, short command_id, std::string recv_buffer, size_t buffer_length) = 0;
};

DECLDIR int load_module(int thread_count = 1, int timer_check_seconds = 30);
DECLDIR void unload_module();
DECLDIR int create_new_client(client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr, time_check_ptr time_check);
DECLDIR bool start_client(int client_id, const std::string& server_ip, short server_port);
DECLDIR bool client_send_data(int client_id, const std::string& send_buff, int send_size);
DECLDIR void close_client(int client_id);



