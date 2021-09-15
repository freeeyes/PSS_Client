#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include "framework.h"
#include "packet_format.h"

//测试PSS TCP Client 客户端
//add by freeeyes

void client_connect(int connect_id)
{
    std::cout << "[client_connect](" << connect_id << ") connect OK" << std::endl;
}
void client_dis_connect(int connect_id, std::string error)
{
    std::cout << "[client_dis_connect](" << connect_id << ")error=" << error << std::endl;
}

void client_recv(int connect_id, const char* buffer, int recv_length)
{
    cpacket_format packet_format;
    packet_format.format_recv_buffer(connect_id, buffer, recv_length);

    std::cout << "[client_recv](" << connect_id << ")recv_length=" << recv_length << std::endl;
}

void time_check(int connect_id, int time_pass_seconds)
{
    std::cout << "[time_check](" << connect_id << ")time_pass_seconds=" << time_pass_seconds << std::endl;
}

void Test_Tcp_Connect()
{
    std::string client_ip = "127.0.0.1";
    int client_id = create_new_client(client_connect, client_dis_connect, client_recv, time_check);

    start_client(client_id, client_ip, 10002);

    //测试发送数据
    char send_buffer[240] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = 0x2101;
    unsigned int client_packet_length = 200;

    std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
    nPos += sizeof(int);
    nPos += 32;
    nPos += 200;

    std::string str_send_buffer;
    str_send_buffer.append(send_buffer, 240);
    client_send_data(client_id, str_send_buffer, 240);
    this_thread::sleep_for(chrono::milliseconds(10));
    //close_client(client_id);

}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_CLOSE_EVENT:
        unload_module();
        return TRUE;
    }

    return TRUE;
}

int main()
{
    load_module(1, 5);

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    Test_Tcp_Connect();

    getchar();

    return 0;
}
