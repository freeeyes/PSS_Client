#include "ASIOClient.h"

CASIOClient::CASIOClient(asio::io_context* io_context, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    : socket_(*io_context), packet_format_(packet_format), packet_dispose_(packet_dispose)
{
}

int CASIOClient::get_tms_logic_id()
{
    int logic_thread_count = App_tms::instance()->Get_Logic_Count();
    return connect_id_ % logic_thread_count;
}

bool CASIOClient::start(int connect_id, const std::string& server_ip, short server_port)
{
    //建立连接
    std::cout << "[CASIOClient::start]connect(" << connect_id << ")" << std::endl;
    connect_id_ = connect_id;
    server_ip_ = server_ip;
    server_port_ = server_port;
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

                auto logic_thread_id = get_tms_logic_id();

                for(auto recv_packet : recv_packet_list_info)
                {
                    App_tms::instance()->AddMessage(logic_thread_id, [this, recv_packet]() {
                        packet_dispose_->do_message(connect_id_, recv_packet);
                        });
                }

                recv_last_timer_ = system_clock::now();
                //继续读
                do_read();
            }
            else
            {
                //链接断开
                //std::cout << "[CASIOClient::do_read]error=" << ec.message() << std::endl;
                auto packet_dispose = packet_dispose_;
                auto connect_id = connect_id_;
                auto recv_error = ec.message();
                App_tms::instance()->AddMessage(get_tms_logic_id(), [connect_id, packet_dispose, recv_error]() {
                    crecv_packet recv_packet;
                    recv_packet.command_id_ = disconnect_command_id;
                    recv_packet.packet_body_ = recv_error;
                    packet_dispose->do_message(connect_id, recv_packet);
                    });
                close_socket();
                //自动重连消息
                App_tms::instance()->AddMessage(self->get_tms_logic_id(), [self]() {
                    //自动重连
                    self->reconnect();
                    });
            }
        });
}

void CASIOClient::do_write_format_data(short command_id, const char* data, size_t length)
{
    std::string send_packet = packet_format_->format_send_buffer(connect_id_, command_id, data, length);
    do_write_immediately(send_packet.c_str(), send_packet.size());
}

void CASIOClient::do_write_immediately(const char* data, size_t length)
{
    auto self(shared_from_this());
    char* send_buffer = new char[length];

    std::memcpy(send_buffer, data, length);

    int connect_id = connect_id_;
    auto packet_dispose = packet_dispose_;
    asio::async_write(socket_, asio::buffer(send_buffer, length),
        [self, send_buffer, connect_id, packet_dispose](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //发送失败
                auto write_error = ec.message();
                App_tms::instance()->AddMessage(self->get_tms_logic_id(), [connect_id, write_error, packet_dispose, self]() {
                    crecv_packet recv_packet;
                    recv_packet.command_id_ = disconnect_command_id;
                    recv_packet.packet_body_ = write_error;
                    packet_dispose->do_message(connect_id, recv_packet);
                    });
                self->close_socket();
                //自动重连消息
                App_tms::instance()->AddMessage(self->get_tms_logic_id(), [self]() {
                    //自动重连
                    self->reconnect();
                    });
            }

            self->set_write_time();
            delete[] send_buffer;
        });
}

void CASIOClient::close_socket()
{
    std::cout << "[CASIOClient::close_socket]connect_id=" << connect_id_ << std::endl;
    is_connect_ = false;
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

        App_tms::instance()->AddMessage(get_tms_logic_id(), [this]() {
                crecv_packet recv_packet;
                recv_packet.command_id_ = connect_command_id;
                packet_dispose_->do_message(connect_id_, recv_packet);
            });

        //更新接收时间
        recv_last_timer_ = system_clock::now();
        
        do_read();
    }
    else
    {
        is_connect_ = false;
        //std::cout << "[CASIOClient::start]error=" << ec.message() << std::endl;
        auto packet_dispose = packet_dispose_;
        auto connect_id = connect_id_;
        auto connect_error = ec.message();
        App_tms::instance()->AddMessage(get_tms_logic_id(), [packet_dispose, connect_id, connect_error]() {
            crecv_packet recv_packet;
            recv_packet.packet_body_ = connect_error;
            recv_packet.command_id_ = disconnect_command_id;
            packet_dispose->do_message(connect_id, recv_packet);
            });
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
    if (is_connect_ == true)
    {
        //只有在链接状态的时候，才发送检测超时消息。
        crecv_packet recv_packet;
        recv_packet.command_id_ = time_check_command_id;
        recv_packet.packet_body_ = std::to_string(seconds);
        packet_dispose_->do_message(connect_id_, recv_packet);
    }
    else
    {
        //重新链接服务器
        reconnect();
    }
}

void CASIOClient::reconnect()
{
    if (false == is_connect_)
    {
        //重连
        std::cout << "[CASIOClient::reconnect]connect(" << connect_id_ << ")" << std::endl;
        tcp::endpoint end_point(asio::ip::address::from_string(server_ip_.c_str()), server_port_);
        asio::error_code connect_error;

        auto handler = std::bind(
            &CASIOClient::connect_handler,
            this,
            std::placeholders::_1);

        //异步链接
        socket_.async_connect(end_point, handler);
    }
    else
    {
        //链接已存在，不需要重连
        std::cout << "[CASIOClient::reconnect]connect(" << connect_id_ << ") is exist" << std::endl;
    }
}

