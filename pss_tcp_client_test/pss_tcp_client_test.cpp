#include <iostream>
#include <chrono>
#include <thread>
#include "framework.h"
#include "packet_format.h"
#include "packet_dispose.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <windows.h>
#endif

//测试PSS TCP Client 客户端
//add by freeeyes

void Test_Tcp_Connect()
{
    std::string client_ip = "116.30.223.246";
    short client_port = 8866;
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();
    packet_dispose->set_packet_format(packet_format);

    int client_id = start_client(client_ip, client_port, packet_format, packet_dispose);

    //close_client(client_id);

}

#if PSS_PLATFORM == PLATFORM_WIN
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
#endif

int main()
{
    load_module(1, 5);

#if PSS_PLATFORM == PLATFORM_WIN
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

    Test_Tcp_Connect();

    getchar();

    return 0;
}
