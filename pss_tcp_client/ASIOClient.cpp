#include "ASIOClient.h"

CASIOClient::CASIOClient(asio::io_context* io_context, client_connect_ptr connect_ptr, client_dis_connect_ptr dis_connect_ptr, client_recv_ptr recv_ptr) : socket_(*io_context), connect_ptr_(connect_ptr), dis_connect_ptr_(dis_connect_ptr), recv_ptr_(recv_ptr)
{
}

bool CASIOClient::start(int connect_id, const std::string& server_ip, short server_port)
{
    //��������
    connect_id_ = connect_id;
    tcp::endpoint end_point(asio::ip::address::from_string(server_ip.c_str()), server_port);
    asio::error_code connect_error;

    socket_.connect(end_point, connect_error);
    if (connect_error)
    {
        //���ӽ���ʧ��
        std::cout << "[CASIOClient::start]error=" << connect_error.message() << std::endl;
        dis_connect_ptr_(connect_id_, connect_error.message());
        return false;
    }
    else
    {
        //���ӽ����ɹ�, ��ʼ��������
        connect_ptr_(connect_id_);

        do_read();

        return true;
    }
}

void CASIOClient::do_read()
{
    auto self(shared_from_this());

    socket_.async_read_some(asio::buffer(recv_buffer, recv_buffer_max_size_),
        [this, self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                //��������
                recv_ptr_(connect_id_, recv_buffer, (int)length);

                //������
                do_read();
            }
            else
            {
                //���ӶϿ�
                //std::cout << "[CASIOClient::do_read]error=" << ec.message() << std::endl;
                dis_connect_ptr_(connect_id_, ec.message());
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
                //����ʧ��
                self->dis_connect_ptr_(connect_id, ec.message());
                self->close_socket();
            }

            delete[] send_buffer;
        });
}

void CASIOClient::close_socket()
{
    socket_.close();
}
