#include "client_manager.h"
#include "framework.h"

//默认定时器的检测间隔

int load_module(int thread_count, int timer_check_seconds)
{
    App_Client_Manager::instance()->run(thread_count, timer_check_seconds);
    return 0;
}

void unload_module()
{
    App_Client_Manager::instance()->close();
}

int start_client(const std::string& server_ip, short server_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
{
    return App_Client_Manager::instance()->start_client(server_ip, server_port, packet_format, packet_dispose);
}

DECLDIR bool reconnect_server(int client_id)
{
    return App_Client_Manager::instance()->reconnect_server(client_id);
}

void close_client(int client_id)
{
    App_Client_Manager::instance()->close_client(client_id);
}

bool client_send_data(int client_id, const std::string& send_buff, int send_size)
{
    return App_Client_Manager::instance()->client_send_data(client_id, send_buff, send_size);
}

DECLDIR bool client_send_format_data(int client_id, short command_id, const std::string& send_buff, int send_size)
{
    return App_Client_Manager::instance()->client_send_format_data(client_id, command_id, send_buff, send_size);
}

