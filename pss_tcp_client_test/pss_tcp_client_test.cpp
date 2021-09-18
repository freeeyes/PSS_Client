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
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();

    int client_id = start_client(client_ip, 10002, packet_format, packet_dispose);

    //测试发送数据
    char body_buffer[200] = { '\0' };
    std::string send_packet = packet_format->format_send_buffer(client_id, 0x2101, body_buffer, 200);

    client_send_data(client_id, send_packet, (int)send_packet.size());
    std::cout << "begin wait recv" << std::endl;
    this_thread::sleep_for(chrono::milliseconds(1000));
    std::cout << "end wait recv" << std::endl;
    close_client(client_id);

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
