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

#ifndef CAN_H_
#define CAN_H_

#include <absl/types/any.h>
#include <vector>
#include "builtin_types.hpp"
#include "OS/os_abstraction.h"
#include "cmi/CanMessage.h"

/**
  \file  CAN.h
  @brief Basic interface to CAN driver.
*/

namespace CanMoveIt
{

typedef std::function<void(const CanMessage &)> CanRcvCallback;
typedef std::function<bool(const CanMessage &)> InterpreterCallback;

/**
 * @ingroup can_interface
 * @brief Class used to interact with a single CAN port.
 *
 * This class is used to send and receive CAN frames on a single node.
 * Nevertheless it is not just a wrapper of the functionalities provided by driver.
 * It is main advantage is that CAN bessages are received using a publish-subscribe model
 * and not polling the port to see if new messages have been received.
 * In fact, multiple clients can be notified when a CAn frame is received using
 * a callback system.
 * Note that each CANport creates its own thread unde the hood and this thread is the one that
 * execute the related callbacks synchronously.
 *
 * */
class CANPort
{

public:
    CANPort();

    ~CANPort();

    /**
     * @brief Open a CAN port.
     * @param busname Name of the device to open (as it is known by the operative system).
     * @param bitrate The bitrate should be either 1M, 500K, 250K, 125K, etc...
     * @return
     *       - 0 if succes
     *       - errorcode if the CAN port can't be opened.
     */
    int16_t open(const char *busname, const char *bitrate);

    /** Get the name of the opened CAN port. */
    const char* busname();

    /**
     * @brief Send a CAN message to the OS driver.
     * @param *m The CAN message to send
     * @return
     * 		- 0 if succes
     * 		- errorcode from the CAN driver is returned if an error occurs. (if implemented in the CAN driver)
     */
    int16_t send(CanMessage *m);

    /**
     * @brief Close a CAN port.
     * @return
     *       - 0 is returned upon success.
     *       - errorcode if error.
     */
    int16_t close();

    /**
     * @brief Get the current state of the CAN port.
     * @return Code of the current status.
     */
    int16_t status();

    /** Check if the port is opened already. */
    bool is_open( ) const;

    /**
     * @brief This method is used to subscribe a callback to a particular message, based on
     *  its frame identifier (the 11 bit header).
     *  The callback is triggered by all the messages "msg" that pass the following test:
     *
     *       if( frame_id == (msg.cob_id & mask ) ) ...
     *
     * If you want to subscribe to ALL the messages, you may use mask=0 and frame_id=0.
     *
     * @param callback    Pointer to a callback.
     * @param mask        The mask to be applied to the incoming messages.
     * @param frame_id    When frames of this kind are received (value is filtered using the mask). Trigger the callback.
     * @return            The pointer to be used with unsubscribeCallback. We use absl::any as
     *                    a safer alternative to void*.
     */
    absl::any  subscribeCallback(CanRcvCallback callback, uint16_t mask,  uint16_t frame_id);

    /**
     * @brief Unsubscribe a callback.
     * It will be recognized among the registered callbacks using the mask and the frame_id used when
     * subscribeCallback was called.
     */
    void unsubscribeCallback(const absl::any &callback);

    void traceSetSize(uint32_t s);

    void traceEnable(bool enable);

    bool tracePop(CanMessage* m, Microseconds timeout);


private:

    class Impl;
    Impl* _d;

    friend bool isCanReadThread();
};

/**
 * @ingroup can_interface
 * @brief The shared pointer that is used to work with the class CANPort.
 * It is returned by the factory openCanPort.
 * */
typedef std::shared_ptr< CANPort > CANPortPtr;


/**
 * @ingroup can_interface
 * @brief A factory of CANPort instances. It will also open the port for you.
 *
 * @param busname  The address used by you operative system to identify the hardware device.
 * @param bitrate   The bitrate should be either 1M, 500K, 250K, 125K, etc...
 * @return
 * 		- 0 if succes
 * 		- errorcode from the CAN driver is returned if an error occurs. (if implemented in the CAN driver)
 */
CANPortPtr openCanPort(const char* busname, const char* bitrate );


/**
 * @ingroup can_interface
 * @brief Unload CAN driver.
 *
 * @return
 *       -  0 is returned upon success.
 *       - -1 is returned if the CAN driver interface can't be unloaded.
 */
int8_t UnLoadCanDriver();

/**
 * @ingroup can_interface
 * @brief Load a dynamic library specific for your hardware (CAN interface on the master).
 * If the file can't be found, an exception is thrown.
 *
 * @param *driver_name  The full name with path of the library to load.
 *
 */
void LoadCanDriver(const char* driver_name);


bool isCanReadThread();


//---------------------------------------------------------------------------------------

} // end namespace

#endif /* CAN_H_ */
