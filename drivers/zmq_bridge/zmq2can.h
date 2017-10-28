#ifndef _ZMQ2CAN_H_
#define _ZMQ2CAN_H_

#include "cmi/CanMessage.h"

#define CAN_MSG_LENGTH 12

namespace CanMoveIt
{

const char* CANWRITE_PORT = "7779";
const char* CANREAD_PORT  = "7778";

inline void Can2Buff(const CanMessage& msg, unsigned char* buffer)
{
    buffer[0] = msg.len;
    buffer[1] = msg.cob_id & 0xFF;
    buffer[2] = (msg.cob_id >> 8) & 0xFF;
    for (int x=0; x<8; x++)
    {
       buffer[3+x] = msg.data[x];
    }
}

inline void Buff2Can(unsigned char* buffer, CanMessage* msg)
{
    msg->len = buffer[0];
    msg->cob_id = buffer[1] + (buffer[2] << 8);
    for (int x=0; x<8; x++)
    {
        msg->data[x] = buffer[3+x];
    }
}

}

#endif
