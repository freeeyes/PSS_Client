#pragma once

#include "framework.h"
#include <thread>

class cpacket_dispose : public ipacket_dispose
{
public:
    cpacket_dispose() = default;

    bool do_message(int connect_id, crecv_packet recv_packet) final;

    void set_packet_format(std::shared_ptr<ipacket_format> packet_format);

private:
    std::shared_ptr<ipacket_format> packet_format_;
};

