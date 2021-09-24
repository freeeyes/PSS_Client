<p align="center"><img src="https://raw.githubusercontent.com/freeeyes/PSS/gh-pages/_images/pss.svg?sanitize=true" alt="pss" width="380"/></p>

Table of Contents
=================

 - [Overview](#overview)
 - [Build and Install](#build-and-install)
 - [how to use](#how-to-use)

Overview
========

This is an integrated IO link library that provides support for tcp, udp, and tty protocols,   
allowing customers to ignore the details of the link, and when the link is broken,   
it will automatically reconnect.  

Build and Install
=================
 * [Notes for WINDOWS-like platforms] use vs2091  
 * [Notes for UNIX-like platforms] use cmake  

how to use
==========

add module init.  
load_module is ceate logic thread and io check time.  
You can specify the number of logical threads.   
When the IO receives a message,  
it will deliver the data to these logical threads for processing in accordance with the load balancing method.
```c++
int main()
{
    load_module(1, 5);

#if PSS_PLATFORM == PLATFORM_WIN
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

    Test_Tcp_Connect();

    getchar();

    return 0;
}
```

Implement two virtual classes,  
one is to format IO to send and receive data,  
and the other is to process and receive messages.
```c++
void Test_Tcp_Connect()
{
    std::string client_ip = "127.0.0.1";
    short client_port = 10002;
    auto packet_format = std::make_shared<cpacket_format>();
    auto packet_dispose = std::make_shared<cpacket_dispose>();

    int client_id = start_client(client_ip, client_port, packet_format, packet_dispose);

    //close_client(client_id);

}
```

When the remote data arrives, process it according to your message ID.  
```c++
bool cpacket_dispose::do_message(int connect_id, crecv_packet recv_packet)
{
    switch (recv_packet.command_id_)
    {
        case 0x0001:
        {
            //connect event
            std::cout << "[event]connect events(" << connect_id << ")." << std::endl;

            //test send message to server
            char body_buffer[200] = { '\0' };
            std::string send_packet;
            send_packet.append(body_buffer, 200);
            client_send_format_data(connect_id, 0x2101, send_packet, (int)send_packet.size());

            break;
        }
        case 0x0002:
        {
            //disconnect
            std::cout << "[event]disconnect events(" << connect_id << "),error=" << recv_packet.packet_body_ << "." << std::endl;
            break;
        }
        case 0x0003:
        {
            //connect timout
            std::cout << "[event]timeout events(" << connect_id << ") timeout second(" << recv_packet.packet_body_ << ")" << std::endl;
            break;
        }
        default:
        {
            //user event
            std::cout << "[event]logic events(" << connect_id << ") command id=" << recv_packet.command_id_ << ", body_length=" << recv_packet.packet_size_ <<"." << std::endl;
            
            //do your job
            break;
        }
    }

    return true;
}
```  