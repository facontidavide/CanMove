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


#ifndef TIMERMANAGER_H_
#define TIMERMANAGER_H_


#include <signal.h>
#include "OS/Thread.h"
#include "absl/types/any.h"

namespace CanMoveIt 
{

/* --------- types and constants definitions --------- */


/**
 * @brief The AsyncManager class is used to create several timers.
 * Each timer is associated to a callback that will be executed asynchronously when (and if)
 * the timer expires.
 * Each instance of AsyncManager will create its own thread, that will be used to executed the callbacks;
 * for this reasons, keep in mind that if any callback shares data with another thread, in particular the one
 * that created the timer using setAlarm, such data shall be protected using a Mutex.
 * Callback are executed serially, consequently if one callback will takes too much time to be executed,
 * it might prevent the following callback to be called at the right time.
 *
 * Note: in CanMoveIt this class is used very often to execute some actions when a timeout
 * is reached.
 * This means that deleting an existing timer (i.e. no timeout) is the most common use cases.
 */
class AsyncManager
{
public:
    typedef struct{} NO_OWN_THREAD;
    /**
        * When the constructor is called, a new thread is allocated and started with
        * a certain priority.
        */
    AsyncManager(uint8_t priority );

    AsyncManager(NO_OWN_THREAD);

    ~AsyncManager();

    /// This is the type of accepted callbacks.
    typedef std::function<void(void)> Callback_t;

    /// You get this handle when you call setAlarm. It can be used to delete an alarm using delAlarm.
    typedef  absl::any  Handle_t;

    /**
     * @brief addAlarm will create an alarm that you can set and delete.
     *  Please note that if the Handle_t (that is actually a shared_ptr) goes out of scope, the alarm is cancelled.
     * @return The handle to be used with setAlarm and delAlarm.
     */
    Handle_t addAlarm();

    /**  setAlarm will is used to generate a signal that trigger the callback after
        * a time equal to _value_ .
        * Note that you can also set a periodic timer.
        *
        */
    void setAlarm(Handle_t alarm, Callback_t cb, Microseconds value);

    /**
        * Delete a specific alarm created with setAlarm
        * @param handle the handle returned by setAlarm.
        */
    void delAlarm(Handle_t hd);


    void addImmediateCallback(Callback_t cb );

    void flush_expired();

    void kill();

private:

    AsyncManager(AsyncManager const&);              // Don't Implement
    void operator=(AsyncManager const&);			// Don't implement

    struct Impl;
    Impl* _d;
};



} /* namespace CanMoveIt */
#endif /* TIMERMANAGER_H_ */
