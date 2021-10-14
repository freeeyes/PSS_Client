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
    std::string client_ip = "127.0.0.1";
    short client_port = 10002;
    auto io_type = em_io_type::IO_TYPE_TCP;
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();

    int client_id = start_client(client_ip, client_port, packet_format, packet_dispose, io_type);

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

    //测试定时器
    add_timer_loop(3, std::chrono::seconds(1), std::chrono::seconds(1), []() {
        std::cout << "time is run" << std::endl;
        });

#if PSS_PLATFORM == PLATFORM_WIN
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

    Test_Tcp_Connect();

    getchar();

    return 0;
}
