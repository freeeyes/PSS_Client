#pragma once

#include <map>
#include <string>

using namespace std;

using client_connect_ptr = void(*)(int);
using client_dis_connect_ptr = void(*)(int, std::string ec);
using client_recv_ptr = void(*)(int, const char* buffer, int recv_length);

//自动判定操作系统
#define PLATFORM_WIN     0
#define PLATFORM_UNIX    1
#define PLATFORM_APPLE   2

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64)
#  define PSS_PLATFORM PLATFORM_WIN
#elif defined(__APPLE_CC__)
#  define PSS_PLATFORM PLATFORM_APPLE
#else
#  define PSS_PLATFORM PLATFORM_UNIX
#endif