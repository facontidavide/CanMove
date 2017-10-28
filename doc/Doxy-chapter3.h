#ifndef DOXYCHAPTER3_H
#define DOXYCHAPTER3_H

/**
 * \page chapter-3 Chapter 3: Asynchronous communication.
 *
 * \tableofcontents
 *
 * One of the main features of CAN/MoveIt is that it is designed to work __asynchronously__.
 * Before describing how this affects your application let's keep in mind that being asynchonous is
 * very important in real-time applcations and when a master needs to control several slaves at the same
 * time.
 *
 * - When commands are send by the master to one of its slaves (for example the reference position of a motor),
 *   the CAN message is actually queued into a FIFO.
 *   A separate thread will try to send that message as soon as possible. From the application point of view,
 *   the method invoked is non-blocking.
 * - If you want some information to be sent by the slave to the master (for example the actual position of the motor)
 *   you need first to send a request and then you have to check locally if a new data sample has been received or not.
 * - You can attach callbacks which are executed either synchonously or asynchornously as soon as a certain data object is updated.
 *   If you use synchronous callbacks you MUST be carefully about thread safety, because the callback is executed in a thread
 *   that is different from the one where your application is running.
 *
 * At first sight, the fact that you don't really know when a message is sent and when a reply is received might look
 * annoying but, in fact, it is the only way to maximum the throughput of your application.
 *
 * Since you are not blocking when you send a request for new data, you can use the time you would have remained
 * blocked to do something else.
 *
 * See for example what happens when you want to read data from multiple devices: on the __left__ you can see the
 * __blocking__ implementation while on the __right__ side there is the same operation executed __asynchonously__, as CMI does.
 *
 * \image html asynch.png
 *
 * For those of you that can't live without a waiting function, CMI provides also __blocking methods__ to send a request
 * and wait for an answer. These methods always need a timeout to be specified; in this way we prevent the user
 * application to block indefinitely if a slave isn't cooperating or if the bus is in fault state.
 * We actually saw already the corresponding API in \ref chapter-2-4.
 *
 *
 *
* <h1>Callbacks and event</h1>
*
* As we have seen, the asynchronous architecture of CMI/Core allows a larger throughput of the messages since it
* allows to send requests to the CANopen networks without blocking (unless the user wants to). This might trigger
* the question: "is there a more effective way than polling to detect when a reply/message has been received?".
*
* The answer is of course "yes", since CMI/Core provides a very simple interface to subscribe to events using
* user-defined callbacks. Let's see a simple example.
*
* \code
*
    // the callback
    void MyCallBack(ObjectEntry const& entry, ObjectData const& data_obj)
    {
        std::cout << "gotcha!" <<std::endl;
    }
*
* \endcode
*
*
* \code
*
    //Code in your application"
    ObjectID statusword( 0x6041, 0); // just an object of CiA402.

    // "motor" is a CO301_Interface pointer
    EventPtr my_event;
    my_event = motor->events()->add_subscription(statusword, CALLBACK_SYNCH, MyCallBack );
    // Send a request. When the reply is received, the callback is executed immediately.
    motor->sdoObjectRequest(statusword);
*
* \endcode
*
* As it can be noticed, we used the parameter __CALLBACK_SYNCH__; this means that the callback will be
* executed as soon as the corresponding reply (i.e. the value of <em>statusword</em>) is received from the CAN bus.
* This approach might look natural and convenient (and most of the times, it is convenient indeed), but there is one
* important pitfalls that the user should understand:
*
* \note Since the callback is executed inside a thread that is different from the one of the user, problems of concurrency
* might occur if the callback access or modify a shared variable. The user must be aware of that and use mutexes
* accordingly or non-blocking, thread-safe data structures.
*
To avoid this kind of problem, the developer is encouraged to use instead the option __CALLBACK_ASYNCH__. Our code will become:
*
* \code
*
    // "motor" is a CO301_Interface pointer
    EventPtr my_event;
    my_event = motor->events()->add_subscription(statusword, CALLBACK_ASYNCH, MyCallBack );
    // Send a request. When the reply is received, the callback is executed immediately.
    motor->sdoObjectRequest(statusword);
    motor->events()->spin( Milliseconds(10) );
*
* \endcode
*
In this way the callback are buffered in a FIFO and will be executed only when the method spin() is called by the user.
We should specify the amount of time that we want to wait for a new callback when the queue is empty.

Contrariwise to the previous approach (the synchronous one), this method is usually thread safe, since the callback are
executed in the same thread that call the method spin().

How many times will the callback be executed? We can control that as described further:
*
* \code
*
    // my_event is the pointer returned by add_subscription.

    // delete the event (remove the subscription).
    motor->events()->eraseEvent( my_event );

    // ...or, better, just disable it:
    motor->events()->configureRepeat( my_event, EVENT_DISABLED );

    // it can be enable again using:
    motor->events()->configureRepeat( my_event, EVENT_ENABLED );

    // ... or execute it only once and then disable it automatically
    motor->events()->configureRepeat( my_event, EVENT_ENABLE_ONCE );
*
* \endcode
*
There is one last event configuration available, that is __CALLBACK_ASYNCH_CANREAD__, which means that the callback will be
executed in the same thread that read a new raw CAN message. This method must be used only when you strictly need to have
the callback executed before a new queued CAN message is sent to the slave. Most of the use case don't need to use this
functionality and developers are discouraged to use it, since it can be relatively dangerous.

In fact, since  no new CAN messages, either incoming or outgoing, can be sent/received until the corresponding callbacks
have been executed, a slow or blocking callback might affect negatively the entire application, or even cause a deadlock.

<h2>Object related callbacks</h2>
In the previous section we have seen a kind of callback that is associated to the reception of a certain CANopen Object
Entry.

The expected callback must have this signature:
*
* \code
*
    typedef boost::function<void(ObjectEntry const&, ObjectData const&)>  ObjectCallback;

    // That is more or less equivalent to:
    // typedef void(*ObjectCallback)(ObjectEntry const&, ObjectData const&);
*
* \endcode
*
As we have seen already, having the ObjectEntry we have access to some meta-information about the object itself
 (the index, sub-index, type, size, etc.) and the value itself stored in ObjectData.

<h2>Generic Events</h2>
There is another class of events that is not directly related to a single ObjectEntry. The signature of the callback is:
*
* \code
*
    typedef boost::function< void(EventData const&)> EventCallback;

    // where EventData has the following fields:
    //
    //EventID       event_id;  // Identifier of the event.
    //absl::any    data;      // Data included in the event.
    //TimePoint     timestamp; // time when the event was received by CanMoveIt.
*
* \endcode
*
Next there is a list of the currently supported __EventID__:

__EVENT_ERROR_IN_PROTOCOL__:
- __Event data__: The error code (see the documentation of your manufacturer).
- __Description__: This is usually sent when you did something illegal in the SDO protocol, for example you tried
                   to access an Object that is not in the dictionary of the device.

__EVENT_EMERGENCY_CO301__
- __Event data__: The error code (see the documentation of your manufacturer).
- __Description__: This messages are standardized in CANopen and represent some important failure of the system
                   (for example a fault in the CAN bus).

__EVENT_SLAVE_FAULT__
- __Event data__: Not supported yet.
- __Description__: The slave moved to a fault state. This is currently detected only in the Motor Abstract Layer.


__EVENT_PROFILED_POSITION_REACHED__
- __Event data__: Not applicable.
- __Description__: Event send when a target position as been reached (MAL).


 *
 */

#endif // DOXYCHAPTER3_H
