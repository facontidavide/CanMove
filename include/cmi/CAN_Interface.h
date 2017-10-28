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

#ifndef CANIntERFACE_H_
#define CANIntERFACE_H_

#include <vector>
#include <list>
#include <map>

#include "OS/AsyncManager.h"
#include "cmi/builtin_types.hpp"
#include "cmi/CAN.h"
#include "cmi/EventDispatcher.h"

namespace CanMoveIt {

/** @defgroup can_interface L1: Generic interface to CAN
 *  @brief This is the first layer to be used to define an object/class that use a CANport
 *  It is designed to work asynchronously by default (non blocking methods).
 */

/**
 *  @ingroup can_interface
 *  @class CanInterface
 *  @brief This class takes care of some write - read operation in either an asynchonous of synchronous way.
 *
 *  When CAN message are sent using pushMessage, they are actually queued inside a FIFO
 * (as the name of the method suggests).
 *  We can tell to the queue that after the transmission of a certain CAN frame, an answer is needed and
 *  we should not send more frames until a certain COBID is received by the slave.
 *  To do that, we should modify the following fields of the CanMessage:
 *
 *  - CanMessage::desired_answer
 *  - CanMessage::wait_answer
 *
 *  For example, when we send a SDO request to a CANopen node (node ID 3) the CanMessage would be:
 *
 *      msg.cob_id         = 0x603;  // COBID used by the SDO client for the request
 *      msg.desired_answer = 0x583;  // COBID used by the SDO server for the reply
 *      msg.wait_answer    = NEED_TO_WAIT_ANSWER;
 *
 * If this answer isn't received after a certain amount of time, a timeout is triggered and the queue will "pop_front" the
 * next message to be sent.
 * This timeout can be modified using the method setReadTimeout.
 *
 *  When a message is received, the callback msgReceivedCallback is executed. The latter method
 *  takes care of dispatching the raw message to one or more callbacks with will do the actual interpretation of the message
 *  using the correct protocol.
 */
class CanInterface
{

public:

    /**
     *  @param can_port This is the port on your PC where the messages will be exchanged. Use canOpen to get it.
     *  @param device_id A _unique_ identifier of the device.
     *  @param subscription_value  Value used as third argument of call CanPort::subscribeCallback.
     *  @param subscription_mask   Mask used as second argument of call CanPort::subscribeCallback.
     */
    CanInterface(CANPortPtr can_port, uint16_t device_id, uint16_t sub_value = 0, uint16_t sub_mask = 0);

    ~CanInterface();

    /** Push a message into the queue . Note that the message isn't delivered immediately. This methos is NON blocking.
    * You can additionally decide to push the message in the fron of the queue instead of the back (default behaviour).
    */
    virtual int pushMessage(const CanMessage & m, bool push_front = false);

    /** Set the value of the default timeout to be used when the queue is waiting for an answer. */
    void setReadTimeout(Microseconds usec);

    /** Get the value of the default timeout to be used when the queue is waiting for an answer. */
    Microseconds getReadTimeout();

    /** Get the last CanMessage successfully sent over the CAN network. */
    CanMessage const& getLastMsgSent();

    /** Get the last CanMessage received from the CAN network. */
    CanMessage const& getLastMsgReceived();

    /// Delete all the queued messages.
    void cleanCanSendBuffer();

    /** Add a callback to be executed when a message is received.
     * It is used _most of the time_ by derived classes to interpret the specific meaning of the message.
     */
    void addReadInterpreter(InterpreterCallback callback);

    /**
     * @brief Block and wait until the queue is empty and no more answer are expected.
     * @param timeout Timeout in microseconds.
     * @return true if everything ok, false if the timeout occurred.
     */
    bool waitQueueEmpty(Microseconds timeout);

    /** This is the pointer of the EventDispatcher associated to this node. */
    EventDispatcher* events();

    /** Return the unique identifier of this interface. */
    uint16_t device_ID() const;

    /** Return the can port. */
    CANPortPtr can_port();


private:

    class Impl;
    Impl* _d;

};



} /* namespace CanMoveIt */

#endif /* CANIntERFACE_H_ */
