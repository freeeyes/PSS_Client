#include "ASIOTTYClient.h"

CASIOTTYClient::CASIOTTYClient(asio::io_context* io_context, std::shared_ptr<ipacket_format> packet_format, std::shared_ptr<ipacket_dispose> packet_dispose)
    :io_context_(io_context), packet_format_(packet_format), packet_dispose_(packet_dispose)
{

}

int CASIOTTYClient::get_tms_logic_id()
{
    int logic_thread_count = App_tms::instance()->Get_Logic_Count();
    return connect_id_ % logic_thread_count;
}

bool CASIOTTYClient::start(int connect_id, const std::string& tty_name, short tty_port)
{
    connect_id_ = connect_id;
    if (false == add_serial_port(tty_name, tty_port))
    {
        return false;
    }

    //更新接收时间
    recv_last_timer_ = system_clock::now();

    tty_name_ = tty_name;
    tty_port_ = tty_port;

    asio::serial_port::baud_rate option;
    std::error_code ec;
    serial_port_param_->get_option(option, ec);
    if (ec)
    {
        //链接异常，退出
        auto packet_dispose = packet_dispose_;
        auto connect_id = connect_id_;
        auto connect_error = ec.message();
        App_tms::instance()->AddMessage(get_tms_logic_id(), [packet_dispose, connect_id, connect_error]() {
            crecv_packet recv_packet;
            recv_packet.packet_body_ = connect_error;
            recv_packet.command_id_ = disconnect_command_id;
            packet_dispose->do_message(connect_id, recv_packet);
            });

        return false;
    }
    else
    {
        //链接正常
        is_connect_ = true;
        
        do_read();
    }

    return true;
}

void CASIOTTYClient::do_read()
{
    serial_port_param_->async_read_some(asio::buffer(recv_buffer, recv_buffer_max_size_),
        [this](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                //处理数据(接收消息)
                auto recv_packet_list_info = packet_format_->format_recv_buffer(connect_id_, recv_buffer, length);

                auto logic_thread_id = get_tms_logic_id();

                for (auto recv_packet : recv_packet_list_info)
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
                auto self(shared_from_this());

                //链接断开
                //std::cout << "[CASIOUDPClient::do_read]error=" << ec.message() << std::endl;
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

void CASIOTTYClient::do_write_format_data(short command_id, const char* data, size_t length)
{
    std::string send_packet = packet_format_->format_send_buffer(connect_id_, command_id, data, length);
    do_write_immediately(send_packet.c_str(), send_packet.size());
}

void CASIOTTYClient::do_write_immediately(const char* data, size_t length)
{
    auto self(shared_from_this());
    char* send_buffer = new char[length];

    std::memcpy(send_buffer, data, length);

    int connect_id = connect_id_;
    auto packet_dispose = packet_dispose_;
    serial_port_param_->async_write_some(asio::buffer(data, length),
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

void CASIOTTYClient::close_socket()
{
    std::cout << "[CASIOUDPClient::close_socket]connect_id=" << connect_id_ << std::endl;
    is_connect_ = false;
    serial_port_param_->close();
}

bool CASIOTTYClient::get_connect_state()
{
    return is_connect_;
}

void CASIOTTYClient::set_write_time()
{
    write_last_timer_ = system_clock::now();
}

int CASIOTTYClient::get_time_pass_seconds()
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

void CASIOTTYClient::do_check_timeout(int seconds)
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

void CASIOTTYClient::reconnect()
{
    if (false == add_serial_port(tty_name_, tty_port_))
    {
        return;
    }

    //更新接收时间
    recv_last_timer_ = system_clock::now();

    asio::serial_port::baud_rate option;
    std::error_code ec;
    serial_port_param_->get_option(option, ec);
    if (ec)
    {
        //链接异常，退出
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
    else
    {
        //链接正常
        is_connect_ = true;

        do_read();
    }
}

bool CASIOTTYClient::add_serial_port(const std::string& tty_name, uint16 tty_port, uint8 char_size /*= 1*/)
{
    std::error_code ec;
    serial_port_param_ = std::make_shared<asio::serial_port>(*io_context_);

    serial_port_param_->open(tty_name, ec);

    if (ec)
    {
        //发送消息给逻辑块
        is_connect_ = false;
        //std::cout << "[CASIOUDPClient::start]error=" << ec.message() << std::endl;
        auto packet_dispose = packet_dispose_;
        auto connect_id = connect_id_;
        auto connect_error = ec.message();
        App_tms::instance()->AddMessage(get_tms_logic_id(), [packet_dispose, connect_id, connect_error]() {
            crecv_packet recv_packet;
            recv_packet.packet_body_ = connect_error;
            recv_packet.command_id_ = disconnect_command_id;
            packet_dispose->do_message(connect_id, recv_packet);
            });

        return false;
    }

    serial_port_param_->set_option(asio::serial_port::baud_rate(tty_port), ec);
    serial_port_param_->set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none), ec);
    serial_port_param_->set_option(asio::serial_port::parity(asio::serial_port::parity::none), ec);
    serial_port_param_->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one), ec);
    serial_port_param_->set_option(asio::serial_port::character_size(char_size), ec);

    return true;
}

