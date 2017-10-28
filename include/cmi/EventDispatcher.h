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
#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#include <map>
#include "absl/types/any.h"
#include "cmi/builtin_types.hpp"
#include "cmi/ObjectDictionary.h"
#include "cmi/ObjectDatabase.h"


namespace CanMoveIt{

/** @ingroup CANopen
 *
 * The mode that a certain event should have. */
typedef enum{
    PRINT_ON_STREAM = 0,    /// Just a print. More ifstream will be supported in the future
    CALLBACK_SYNCH  = 2,    /// Deliver the event synchronously to a callback (specified by the user).
    CALLBACK_ASYNCH = 3,     /// Push on the FIFO that the user must poll asynchronously.
    CALLBACK_SYNCH_CANREAD = 1
}EventMode;

/** @ingroup CANopen
 * This is the type "hidden" inside EventData::info when you subscribe
 * to an event using an ObjectID.
 *  You need to cast it back from CMY::any usign absl::any_cast<EventDataObjectUpdated>()
 */
class EventDataObjectUpdated{
public:
    const ObjectEntry& entry;
    const ObjectData& data;
    EventDataObjectUpdated( const ObjectEntry& e, const ObjectData& d): entry(e), data(d){}
};

/** @ingroup CANopen
*/
typedef uint32_t EventID;


/** @ingroup CANopen
 * These are some basic events associated to the CanOpen 301 layer.
 **/
const EventID  EVENT_ERROR_IN_PROTOCOL(1);    /// SDO Command not recognized. EventData::data will contain the error code.
const EventID  EVENT_EMERGENCY_FAULT(2);      /// Emergency message received. EventData::data will contain the error code.


///** @ingroup CANopen
// **/
class EventData
{
public:
    EventID       event_id;         ///< Identifier of the event.
    absl::any      info;             ///< Data included in the event.
    TimePoint     timestamp;        ///< time in usec when the event was received by CanMoveIt.

    void print() const
    {
        printf("Event received with id: %d\n", (int)(event_id) );
    }
};

typedef std::function< void(uint16_t device_id, EventData const&)> EventCallback;

typedef enum {
    EVENT_ENABLED     = 1,
    EVENT_DISABLED    = 0,
    EVENT_ENABLE_ONCE = 3
} EventRepeat;

typedef absl::any EventPtr;

/** @ingroup CANopen
 * EventDispatcher is the class used by the application can publish and subscribe to events.
 * Currently two kinds of event are supported:
 * - Events triggered when a certain Object in the CO301 layer us received.
 * - Other special events, related to either C0301 or MAL402.
 *
 * When an event is created, it should be pushed into this class using push_event.
 * Some data can be delivered with the event. See the methods push_event for more details.
 * According to the way that the event was configured the event will be dispatcher to either:
 *
 * - the screen (PRINT_ON_STREAM option)
 * - a callback called synchronously. In this case user is responsible for thread safety (CALLBACL_SYNCH).
 * - a callback called asynchronously. In this case the callback is executed inside the thread where spin() is called.
 *
 * Note that this class is just a convenient wrapper to two instances of the template based class EventDispatcherImpl.
 **/
class EventDispatcher
{

public:

    EventDispatcher();

    ~EventDispatcher();

    /**
     * @brief Add an event that will be raised when something special occurs.
     * This can be any kind of genereic and custom event.
     *
     * @param id        The identifier of the object is just a number that should be unique.
     * @param mode      See EventMode for the different options.
     * @param callback  The callback to be executed. It the pointer is null (0) and the mode is a CALLBACK, a
     *                   exception will we thrown.
     * @return The identifier to be used to reconfigure the event or erase it.
     */
    EventPtr add_subscription(EventID const& id, EventMode const& mode, EventCallback callback );

    /**
     * @brief Use this method to reconfigure an event previously created with add_subscription.
     * @param event  The pointer returned by add_subscription.
     * @param mode   The new mode.
     */
    void configureMode(EventPtr event,EventMode mode);

    /** Modify the callback previously added with add_subscription. */
    void configureCallback(EventPtr event, EventCallback  callback );

    /**
     * @brief Use this method to reconfigure an event previously created with add_subscription.
     * @param event  The pointer returned by add_subscription.
     * @param rep    The new parameter.
     */
    void configureRepeat(EventPtr event,EventRepeat rep );

    /**
     * @brief Delete a previously created subscription.
     *
     * If this delation is not permanent, consider using configureRepeat(event, EVENT_DISABLED ).
     *
     * @param event  The pointer returned by add_subscription.
     */
    void eraseEvent(EventPtr event );

    void eraseEvents(EventID const& id);

    /**
     * When an special event takes place, it must be pushed using this method.
     *
     * @param device_id   Unique identifier of the device.
     * @param data        EventData will be passed as argument to the callback.
     */
    void push_event(uint16_t device_id,const EventData & data);

    /**
     * @brief This method MUST be used together with CALLBACK_ASYNCH.
     * In fact, it will flush _all_ the callbacks that where stored into the queue of
     * asynchronous events.
     */
    void spin(Microseconds ms = Microseconds(0));

protected:

    struct Impl;
    Impl *_d;

};



}// end namespace



#endif // EVENTDISPATCHER_H
