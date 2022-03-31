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

            //测试发送数据
            char body_buffer[200] = { '\0' };
            std::string send_packet;
            send_packet.append(body_buffer, 200);
            client_send_format_data(connect_id, 0x2101, send_packet, (int)send_packet.size());

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

            //测试发送数据
            char body_buffer[200] = { '\0' };
            std::string send_packet;
            send_packet.append(body_buffer, 200);
            client_send_format_data(connect_id, 0x2101, send_packet, (int)send_packet.size());

            break;
        }
        default:
        {
            //业务逻辑消息
            std::cout << "[event]logic events(" << connect_id << ") command id=" << recv_packet.command_id_ << ", body_length=" << recv_packet.packet_size_ <<"." << std::endl;
            
            /*
            //测试发送数据
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            char body_buffer[200] = { '\0' };
            std::string send_packet;
            send_packet.append(body_buffer, 200);
            client_send_format_data(connect_id, 0x2101, send_packet, (int)send_packet.size());
            */
            
            break;
        }
    }

    return true;
}
