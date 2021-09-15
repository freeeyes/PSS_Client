#pragma once

#include <map>
#include <string>
#include <functional>
#include <vector>

using namespace std;

using task_function = std::function<void()>;

class crecv_packet
{
public:
    short command_id_ = 0;
    std::string packet_body_;
    size_t packet_size_ = 0;
};

using recv_packet_list = std::vector<crecv_packet>;

//数据解析组包类(需要上层逻辑提供实现类)
class ipacket_format
{
public:
    virtual recv_packet_list format_recv_buffer(int connect_id, const char* recv_buffer, size_t buffer_length) = 0;
    virtual std::string format_send_buffer(int connect_id, short command_id, std::string recv_buffer, size_t buffer_length) = 0;
};

//数据处理类(需要上层逻辑提供实现类)
class ipacket_dispose
{
public:
    virtual bool do_message(int connect_id, crecv_packet recv_packet) = 0;
};

//基础类型定义
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

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