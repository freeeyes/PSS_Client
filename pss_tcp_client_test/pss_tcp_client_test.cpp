#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "framework.h"
#include "packet_format.h"
#include "packet_dispose.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <windows.h>
#endif

//测试PSS TCP Client 客户端
//add by freeeyes

std::mutex m;
std::condition_variable cv;

void Test_Tcp_Connect()
{
    std::string client_ip = "127.0.0.1";
    short client_port = 10002;
    auto io_type = em_io_type::IO_TYPE_TCP;
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();

    int client_id = start_client(client_ip, client_port, packet_format, packet_dispose, io_type);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /*
    add_local_message(1, []() {
        std::cout << "[local message] is done" << std::endl;
        });
     */
    //close_client(client_id);

    //测试定时器
    int timer_id = add_timer_loop(0, std::chrono::seconds(0), std::chrono::seconds(5), [client_id]() {
        std::time_t now = std::time(nullptr);
        struct tm tm_now;
        localtime_s(&tm_now, &now);
        std::cout << "[" << std::put_time(&tm_now, "%Y-%m-%d %H.%M.%S") << "]" << "time is run" << std::endl;
        });

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
        cv.notify_one();
        return TRUE;
    }

    return TRUE;
}
#endif

int main()
{
    load_module(3, 5);

#if PSS_PLATFORM == PLATFORM_WIN
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

    Test_Tcp_Connect();

    //getchar();
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk);
    lk.unlock();

    return 0;
}
