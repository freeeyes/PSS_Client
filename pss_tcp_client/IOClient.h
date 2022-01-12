#ifndef IO_CLIENT_H
#define IO_CLIENT_H

//定义IO客户端接口类
//add by freeeyes

#include <string>

//默认的命令id
const short connect_command_id = 0x0001;
const short disconnect_command_id = 0x0002;
const short time_check_command_id = 0x0003;

const int recv_buffer_max_size_ = 10240;

class IIOClient
{
public:
    virtual bool start(int connect_id, const std::string& server_ip, short server_port) = 0;
    virtual void do_write_format_data(short command_id, const char* data, size_t length) = 0;
    virtual void do_write_immediately(const char* data, size_t length) = 0;
    virtual void close_client_socket() = 0;
    virtual void do_check_timeout(int seconds) = 0;
    virtual void reconnect() = 0;
    virtual int get_time_pass_seconds() = 0;
};

#endif
