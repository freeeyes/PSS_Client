#include "packet_format.h"

recv_packet_list cpacket_format::format_recv_buffer(int connect_id, const char* recv_buffer, size_t buffer_length)
{
    recv_packet_list recv_packet_list_info;

    //������հ��������ݰ�ճ��
    if (curr_buffer_size_ + buffer_length >= MAX_BUFFER_SIZE)
    {
        std::cout << "[cpacket_format::format_recv_buffer]buffer is full." << std::endl;
        return recv_packet_list_info;
    }

    //����յ���������
    std::memcpy(&recv_buffer_[curr_buffer_size_], recv_buffer, buffer_length);
    curr_buffer_size_ += buffer_length;

    //��ʼѭ���������ݰ�
    while(true)
    {
        if (PACKET_HEAD_SIZE > curr_buffer_size_)
        {
            //û�������İ�ͷ����������
            return recv_packet_list_info;
        }
        else
        {
            //��ͷ�������жϰ����Ƿ�����
            int packet_body_size = 0;
            int command_id = 0;
            std::memcpy(&command_id, &recv_buffer_[2], (short)sizeof(short));
            std::memcpy(&packet_body_size, &recv_buffer_[4], (int)sizeof(int));
            
            if (MAX_PACKET_BUFFER_SIZE <= packet_body_size)
            {
                //���ݰ��峤�ȳ�������󳤶�(������Ч���ݰ�)
                std::cout << "[cpacket_format::format_recv_buffer]packet_body_size is more than default." << std::endl;

                //�����ݻ����������
                curr_buffer_size_ = 0;

                return recv_packet_list_info;
            }
            else
            {
                size_t curr_packet_size = packet_body_size + PACKET_HEAD_SIZE;
                if (curr_packet_size <= curr_buffer_size_)
                {
                    //���������ݰ�����������
                    //std::cout << "[cpacket_format::format_recv_buffer]recv command=" << command_id << ",body length=" << packet_body_size << "." << std::endl;
                    
                    crecv_packet recv_packet;
                    recv_packet.command_id_ = command_id;
                    recv_packet.packet_body_.append(&recv_buffer_[PACKET_HEAD_SIZE], packet_body_size);
                    recv_packet.packet_size_ = packet_body_size;
                    recv_packet_list_info.emplace_back(recv_packet);

                    //�ƶ�����
                    std::memmove(&recv_buffer_[0], &recv_buffer_[curr_packet_size], curr_buffer_size_ - curr_packet_size);
                    curr_buffer_size_ -= curr_packet_size;
                    if (curr_buffer_size_ == 0)
                    {
                        return recv_packet_list_info;
                    }
                }
                else
                {
                    //���ݰ����ղ���������������
                    return recv_packet_list_info;
                }
            }

        }
    }
    
    return recv_packet_list_info;
}

std::string cpacket_format::format_send_buffer(int connect_id, short command_id, std::string recv_buffer, size_t buffer_length)
{
    std::string client_send_packet;
    if (buffer_length + PACKET_HEAD_SIZE > MAX_PACKET_SEND_SIZE)
    {
        std::cout << "[cpacket_format::format_send_buffer]send buffer is full." << std::endl;
        return client_send_packet;
    }

    unsigned short client_version = 1;
    unsigned short client_command_id = command_id;
    unsigned int client_packet_length = (unsigned int)buffer_length;
    size_t nPos = 0;

    std::memcpy(&send_buffer_[nPos], &client_version, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer_[nPos], &client_command_id, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer_[nPos], &client_packet_length, sizeof(int));
    nPos += sizeof(int);
    nPos += 32;
    std::memcpy(&send_buffer_[nPos], recv_buffer.c_str(), buffer_length);
    nPos += buffer_length;

    client_send_packet.append(send_buffer_, nPos);
    return client_send_packet;
}
