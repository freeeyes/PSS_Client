#include "packet_dispose.h"

bool cpacket_dispose::do_message(int connect_id, crecv_packet recv_packet)
{
    //������������¼�
    switch (recv_packet.command_id_)
    {
        case 0x0001:
        {
            //���������¼�
            std::cout << "[event]connect events(" << connect_id << ")." << std::endl;
            break;
        }
        case 0x0002:
        {
            //���ӶϿ��¼�
            std::cout << "[event]disconnect events(" << connect_id << "),error=" << recv_packet.packet_body_ << "." << std::endl;
            break;
        }
        case 0x0003:
        {
            //��Ϣ��ʱ�¼�
            std::cout << "[event]timeout events(" << connect_id << ") timeout second(" << recv_packet.packet_body_ << ")" << std::endl;
            break;
        }
        default:
        {
            //ҵ���߼���Ϣ
            std::cout << "[event]logic events(" << connect_id << ") command id=" << recv_packet.command_id_ << ", body_length=" << recv_packet.packet_size_ <<"." << std::endl;
            break;
        }
    }

    return true;
}
