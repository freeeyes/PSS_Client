#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include "framework.h"
#include "packet_format.h"
#include "packet_dispose.h"

//测试PSS TCP Client 客户端
//add by freeeyes

void Test_Tcp_Connect()
{
    std::string client_ip = "127.0.0.1";
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();

    int client_id = create_new_client(packet_format, packet_dispose);

    start_client(client_id, client_ip, 10002);

    //测试发送数据
    char body_buffer[200] = { '\0' };
    std::string send_packet = packet_format->format_send_buffer(client_id, 0x2101, body_buffer, 200);

    client_send_data(client_id, send_packet, send_packet.size());
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
