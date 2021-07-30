#include <iostream>
#include "framework.h"
#include <windows.h>

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
    std::cout << "[client_recv](" << connect_id << ")recv_length=" << recv_length << std::endl;
}

void Test_Tcp_Connect()
{
    std::string client_ip = "127.0.0.1";
    create_new_client(1, client_connect, client_dis_connect, client_recv);

    start_client(1, client_ip, 10002);

    close_client(1);
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
    load_module(4);

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    Test_Tcp_Connect();

    getchar();

    return 0;
}
