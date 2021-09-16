#include "packet_dispose.h"

bool cpacket_dispose::do_message(int connect_id, crecv_packet recv_packet)
{
    //处理监听到的事件
    switch (recv_packet.command_id_)
    {
        case 0x0001:
        {
            //建立连接事件
            std::cout << "[event]connect events(" << connect_id << ")." << std::endl;
            break;
        }
        case 0x0002:
        {
            //连接断开事件
            std::cout << "[event]disconnect events(" << connect_id << "),error=" << recv_packet.packet_body_ << "." << std::endl;
            break;
        }
        case 0x0003:
        {
            //消息超时事件
            std::cout << "[event]timeout events(" << connect_id << ") timeout second(" << recv_packet.packet_body_ << ")" << std::endl;
            break;
        }
        default:
        {
            //业务逻辑消息
            std::cout << "[event]logic events(" << connect_id << ") command id=" << recv_packet.command_id_ << ", body_length=" << recv_packet.packet_size_ <<"." << std::endl;
            break;
        }
    }

    return true;
}
