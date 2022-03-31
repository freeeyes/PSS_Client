﻿#include <iostream>
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
    //std::this_thread::sleep_for(std::chrono::seconds(3));
    //close_client(client_id);

    //测试定时器
    //int timer_id = add_timer_loop(0, std::chrono::seconds(1), std::chrono::seconds(1), []() {
    //    std::cout << "time is run" << std::endl;
    //    });

    //std::this_thread::sleep_for(std::chrono::seconds(5));

    //close_timer_id(timer_id);
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

//连接发送数据
static void connSnd(int iConnSnd)
{
    std::string client_ip = "127.0.0.1";
    short client_port = 10002;
    auto io_type = em_io_type::IO_TYPE_TCP;
    //建立连接
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();
    int client_id = start_client(client_ip, client_port, packet_format, packet_dispose, io_type);

    /*
    uint64_t iIndex = 0;
    char cData[1024] = { 0 };
    
    while (true)
    {
        sprintf(cData, "iConnSnd=%d iIndex=%lld hllo word!", iConnSnd, ++iIndex);
        string sSendData = cData;
        if (!client_send_format_data(client_id, 0x1001, sSendData, sSendData.size()))
        {
            printf("[%s] %d  client_id[%d] command_id[%d] client_send_format_data[%s] faile.\n", __FUNCTION__, __LINE__, client_id, CMD_OTHER_ID, sSendData.c_str());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//std::chrono::seconds(3)
    }
    */
}

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
