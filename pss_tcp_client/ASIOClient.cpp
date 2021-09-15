#include "ASIOClient.h"

CASIOClient::CASIOClient(asio::io_context* io_context, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    : socket_(*io_context), packet_format_(packet_format), packet_dispose_(packet_dispose)
{
}

bool CASIOClient::start(int connect_id, const std::string& server_ip, short server_port)
{
    //建立连接
    std::cout << "[CASIOClient::start]connect(" << connect_id << ")" << std::endl;
    connect_id_ = connect_id;
    tcp::endpoint end_point(asio::ip::address::from_string(server_ip.c_str()), server_port);
    asio::error_code connect_error;

    auto handler = std::bind(
        &CASIOClient::connect_handler,
        this,
        std::placeholders::_1);

    //异步链接
    socket_.async_connect(end_point, handler);

    return true;
}

void CASIOClient::do_read()
{
    auto self(shared_from_this());

    socket_.async_read_some(asio::buffer(recv_buffer, recv_buffer_max_size_),
        [this, self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                //处理数据(发送消息)
                auto recv_packet_list_info = packet_format_->format_recv_buffer(connect_id_, recv_buffer, length);

                for(auto recv_packet : recv_packet_list_info)
                {
                    packet_dispose_->do_message(connect_id_, recv_packet);
                }

                recv_last_timer_ = system_clock::now();
                //继续读
                do_read();
            }
            else
            {
                //链接断开
                //std::cout << "[CASIOClient::do_read]error=" << ec.message() << std::endl;
                crecv_packet recv_packet;
                recv_packet.command_id_ = disconnect_command_id;
                packet_dispose_->do_message(connect_id_, recv_packet);
                close_socket();
            }
        });
}

void CASIOClient::do_write_immediately(const char* data, size_t length)
{
    auto self(shared_from_this());
    char* send_buffer = new char[length];

    std::memcpy(send_buffer, data, length);

    int connect_id = connect_id_;
    asio::async_write(socket_, asio::buffer(data, length),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //发送失败
                crecv_packet recv_packet;
                recv_packet.command_id_ = disconnect_command_id;
                self->packet_dispose_->do_message(connect_id, recv_packet);
                self->close_socket();
            }

            self->set_write_time();
            delete[] send_buffer;
        });
}

void CASIOClient::close_socket()
{
    socket_.close();
}

bool CASIOClient::get_connect_state()
{
    return is_connect_;
}

void CASIOClient::connect_handler(const asio::error_code& ec)
{
    if (!ec)
    {
        is_connect_ = true;

        crecv_packet recv_packet;
        recv_packet.command_id_ = connect_command_id;
        packet_dispose_->do_message(connect_id_, recv_packet);
        
        do_read();

    }
    else
    {
        is_connect_ = false;
        std::cout << "[CASIOClient::start]error=" << ec.message() << std::endl;
        crecv_packet recv_packet;
        recv_packet.command_id_ = disconnect_command_id;
        packet_dispose_->do_message(connect_id_, recv_packet);
    }
}

void CASIOClient::set_write_time()
{
    write_last_timer_ = system_clock::now();
}

int CASIOClient::get_time_pass_seconds()
{
    auto recv_pass = std::chrono::duration_cast<std::chrono::seconds>(system_clock::now() - recv_last_timer_);
    auto write_pass = std::chrono::duration_cast<std::chrono::seconds>(system_clock::now() - recv_last_timer_);
    if (write_pass > recv_pass)
    {
        return (int)write_pass.count();
    }
    else
    {
        return (int)recv_pass.count();
    }
}

void CASIOClient::do_check_timeout(int seconds)
{
    crecv_packet recv_packet;
    recv_packet.command_id_ = time_check_command_id;
    recv_packet.packet_body_ = std::to_string(seconds);
    packet_dispose_->do_message(connect_id_, recv_packet);
}

