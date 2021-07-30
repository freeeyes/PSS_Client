#include "client_manager.h"
#include "framework.h"

int load_module(int thread_count)
{
    App_Client_Manager::instance()->run(thread_count);
    return 0;
}

void unload_module()
{
    App_Client_Manager::instance()->close();
}

bool create_new_client(int client_id, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr)
{
    return App_Client_Manager::instance()->create_new_client(client_id, connect_ptr, dis_connect_ptr, recv_ptr);
}

bool start_client(int client_id, const std::string& server_ip, short server_port)
{
    return App_Client_Manager::instance()->start_client(client_id, server_ip, server_port);
}

void close_client(int client_id)
{
    App_Client_Manager::instance()->close_client(client_id);
}

bool client_send_data(int client_id, const std::string& send_buff, int send_size)
{
    return App_Client_Manager::instance()->client_send_data(client_id, send_buff, send_size);
}

