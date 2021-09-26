#pragma once

#include "framework.h"
#include <cstring>

const size_t MAX_BUFFER_SIZE = 20480;  //最大buffer缓冲长度
const size_t PACKET_HEAD_SIZE = 2 * sizeof(short) + sizeof(int) + 32 * sizeof(char);
const size_t MAX_PACKET_BUFFER_SIZE = 2048;    //一个最大包体长度
const size_t MAX_PACKET_SEND_SIZE = 2048;   //最大的发包长度

class cpacket_format : public ipacket_format
{
public:
    cpacket_format() = default;

    recv_packet_list format_recv_buffer(int connect_id, const char* recv_buffer, size_t buffer_length) final;

    std::string format_send_buffer(int connect_id, short command_id, std::string recv_buffer, size_t buffer_length, size_t& format_length) final;

private:
    char recv_buffer_[MAX_BUFFER_SIZE] = {'\0'};
    char send_buffer_[MAX_PACKET_SEND_SIZE] = { '\0' };
    size_t curr_buffer_size_ = 0;
};

