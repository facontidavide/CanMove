/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/

#ifndef CAN_MESSAGE_H_
#define CAN_MESSAGE_H_

/**
  \file  CAN.h
  @brief Definition of the CAN message type
*/

#include <iostream>
#include "OS/os_abstraction.h"
#include "cmi/log.h"

namespace CanMoveIt
{


 /** @ingroup can_interface
 * @brief Parameter to be used with CanMessage::wait_answer.
 */
enum {
    NO_WAIT = 0,
    NEED_TO_WAIT_ANSWER  = 1
};


/**
 * @ingroup can_interface
 * @brief This class contains the COB_ID and data but also some extra info about
 * what the message sender should do.
 *
 * In particular, it is possible to ask the sender to wait for a specific answer (identified by its __COB__) before
 * sending the next message in the queue.
 * Note that the class CAN_Interface defines a timeout and will unlock after some time if the answer isn't received from the slave.
 * A "simple send and forget" message is sent when
 *
 *     wait_answer = NO_WAIT (default)
 *
 * SDO are an example of messages which __do need__ an answer. For example SDO downloads need the following code:
 *
 *     cob_id         = SDO_RX + getNode();
 *     wait_answer    = NEED_TO_WAIT_ANSWER;
 *     desired_answer = SDO_TX + getNode();
 *
 */
class CanMessage
{

public:

    union{
        uint32_t raw_storage;
        struct{
            /// @brief Remote transmission request. (0 if not rtr message, 1 otherwise).
            uint32_t rtr               : 1;
            /// @brief Length of the Message (0 to 8 bytes)
            uint32_t len               : 4;

             /// @brief Message's ID (we use only the 11 bit version here).
            uint32_t cob_id            : 11;

            /// @brief COB of the expected answer.
            /// If wait_answer == NEED_TO_WAIT_ANSWER, the class CAN_Interface will wait for a reply with
            /// cob_id equal to desired_answer.
            uint32_t desired_answer    : 11;

            /// @brief Tell the can sended to stop and wait for an answer.
            uint32_t wait_answer       : 2;

            uint32_t received          : 1;
            uint32_t sent              : 1;
        };
    };

    /// @brief Data of the message.
    uint8_t  data[8];

    /// @brief Placeholder for a timestamp.
    /// This can't be a CanMoveIt::TimePoint for technical reasons, therefore we use time_since_epoch
    /// instead and we store is microseconds
    uint64_t timestamp_usec;

    CanMessage();

    /// @brief Cleanup the field of the CanMessage.
    void clear();

    ///  @brief Extract the node ID (7 LSB) from the cob_id.
    uint8_t  getNode()   const { return (cob_id & 0x007F); }

    ///  @brief Extract he message COB (4 MSB) from the cob_id.
    uint16_t getCOB()    const { return (cob_id & 0xFF80); }

    ///  @brief Pretty print of the field of CanMessage.
    void print( ) const;

    void sprint(char* str) const;

    template<typename OStream>
    friend OStream& operator<<(OStream& os, const CanMessage &msg )
    {
        char temp[100];
        msg.sprint(temp);
        return os << temp;
    }
};



/**
 * @ingroup can_interface
 * @ Handle to be used to open, close, write and read a specific CAN bus.
 * */
typedef struct{
    int   fd;
    void* vp;
}CAN_Handle_t;

} // end namespace


#endif /* CAN_MESSAGE_H_ */
