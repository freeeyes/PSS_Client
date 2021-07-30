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

DECLDIR int load_module(int thread_count = 1);
DECLDIR void unload_module();
DECLDIR bool create_new_client(int client_id, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr);
DECLDIR bool start_client(int client_id, const std::string& server_ip, short server_port);
DECLDIR bool client_send_data(int client_id, const std::string& send_buff, int send_size);
DECLDIR void close_client(int client_id);



