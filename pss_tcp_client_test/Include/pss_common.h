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

//���ݽ��������(��Ҫ�ϲ��߼��ṩʵ����)
class ipacket_format
{
public:
    virtual recv_packet_list format_recv_buffer(int connect_id, const char* recv_buffer, size_t buffer_length) = 0;
    virtual std::string format_send_buffer(int connect_id, short command_id, std::string recv_buffer, size_t buffer_length) = 0;
};

//���ݴ�����(��Ҫ�ϲ��߼��ṩʵ����)
class ipacket_dispose
{
public:
    virtual bool do_message(int connect_id, crecv_packet recv_packet) = 0;
};

//�������Ͷ���
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

//�Զ��ж�����ϵͳ
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