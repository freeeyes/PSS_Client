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

int start_client(const std::string& server_ip, short server_port, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose, em_io_type io_type)
{
    if (em_io_type::IO_TYPE_TCP == io_type)
    {
        //tcp链接
        return App_Client_Manager::instance()->start_client_tcp(server_ip, server_port, packet_format, packet_dispose);
    }
    else if (em_io_type::IO_TYPE_UDP == io_type)
    {
        //udp链接
        return App_Client_Manager::instance()->start_client_udp(server_ip, server_port, packet_format, packet_dispose);
    }
    else if (em_io_type::IO_TYPE_TTY == io_type)
    {
        //tty链接
        return App_Client_Manager::instance()->start_client_tty(server_ip, server_port, packet_format, packet_dispose);
    }
    else
    {
        return false;
    }
}

bool reconnect_server(int client_id)
{
    return App_Client_Manager::instance()->reconnect_server(client_id);
}

void close_client(int client_id)
{
    App_Client_Manager::instance()->close_client(client_id);
}

bool add_timer(int work_thread_id, task_function func)
{
    return App_tms::instance()->AddMessage(work_thread_id, func);
}

DECLDIR bool add_timer_loop(int work_thread_id, std::chrono::milliseconds millisecond, task_function func)
{
    App_tms::instance()->AddMessage(work_thread_id, millisecond, func);
    return true;
}

bool client_send_data(int client_id, const std::string& send_buff, int send_size)
{
    return App_Client_Manager::instance()->client_send_data(client_id, send_buff, send_size);
}

DECLDIR bool client_send_format_data(int client_id, short command_id, const std::string& send_buff, int send_size)
{
    return App_Client_Manager::instance()->client_send_format_data(client_id, command_id, send_buff, send_size);
}

