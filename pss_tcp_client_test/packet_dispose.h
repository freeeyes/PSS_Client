#pragma once

#include "framework.h"

class cpacket_dispose : public ipacket_dispose
{
public:
    cpacket_dispose() = default;

    bool do_message(int connect_id, crecv_packet recv_packet) final;
};

