/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *******************************************************/


#include <cerrno>
#include <boost/lockfree/queue.hpp>
#include "cmi/CAN_Interface.h"
#include "cmi/ObjectDictionary.h"
#include "cmi/globals.h"
#include "cmi/log.h"
#include "OS/AsyncManager.h"

int _cmi_log_level_ = 0;

namespace CanMoveIt
{

class CanInterface::Impl
{
public:

    uint16_t     device_id;
    uint8_t		 node_id;

    // Note: the mutex is not used to push/pop form the can_write_fifo queue, but
    // it is needed by condition_queue_empty
    Mutex        fifo_mutex;
    boost::lockfree::queue<CanMessage>  can_write_fifo;

    typedef enum{ WAITING, WAITING_ANSWER, DONT_WAIT} WaitingState;

    CANPortPtr				 can_port;

    AsyncManager::Handle_t	 timeout_handle;
    Microseconds			 reply_timeout;
    absl::any                subscriber;


    WaitingState    last_msg_wait_answer;
    uint32_t        num_msg_sent;
    uint32_t        num_msg_received;
    CanMessage      last_msg_received;
    CanMessage      last_msg_sent;
    TimePoint       queue_wait_abs_timeout;

    std::vector<InterpreterCallback>  interpreters_list;
    EventDispatcher  event_dispatcher;

    AsyncManager *async_can;

    /** Try send message is non blocking. It will be called when:
     * - a new message is pushed using pushMessage.
     * - a timeout expired.
     * - a message was read AND interpreted.
    */
    void trySendMessage();

    /** This callback is executed in the thread that reads from CAN the raw messages.
    It delegates real work to msgReceivedCallback_Async*/
    void msgReceivedCallback_Sync(const CanMessage & m);

    /** This callback is executed in the thread of _d->async_can->
     * It calls more callbacks (the interpreters_list).
     * We do not know how long these interpreters will take.
     */
    void msgReceivedCallback_Async(const CanMessage m);

    /** Non blocking*/
    void timeout_callback(const char* msg);

    /** Added to make the access at interpreters_list thread safe. */
    void AddReadInterpreter_Async(InterpreterCallback call);


    Impl(): can_write_fifo(50),
        last_msg_wait_answer (DONT_WAIT),
        num_msg_sent(0),
        num_msg_received(0),
        queue_wait_abs_timeout( TimePoint::max() ),
        async_can( &CMI::get().async_can )
    {}
};

CanInterface::CanInterface(CANPortPtr can_port, uint16_t device_id , uint16_t sub_value, uint16_t sub_mask):
    _d( new Impl )
{
    _d->device_id  = device_id;
    _d->can_port   = can_port;
    _d->reply_timeout  = Milliseconds(200);
    _d->timeout_handle = _d->async_can->addAlarm();

    CanRcvCallback callback =  std::bind( &CanInterface::Impl::msgReceivedCallback_Sync, _d, std::placeholders::_1 );
    _d->subscriber = _d->can_port->subscribeCallback( callback, sub_mask, sub_value );

}

CanInterface::~CanInterface()
{
    _d->can_port->unsubscribeCallback( _d->subscriber  );
    delete _d;
}

EventDispatcher* CanInterface::events()
{
    return &_d->event_dispatcher;
}


void CanInterface::setReadTimeout(Microseconds usec)
{
    _d->reply_timeout = usec;
}

Microseconds CanInterface::getReadTimeout()
{
    return _d->reply_timeout ;
}

CanMessage const& CanInterface::getLastMsgSent()
{
    return  _d->last_msg_sent;
}

CanMessage const& CanInterface::getLastMsgReceived()
{
    return  _d->last_msg_received;
}

void CanInterface::cleanCanSendBuffer()
{
    CanMessage tmp;
    //even if this is thread safe, we need the lock to make happy the
    // condition variable inside waitQueueEmpty
    LockGuard t( _d->fifo_mutex) ;
    while( _d->can_write_fifo.pop( tmp ) );
}

uint16_t CanInterface::device_ID() const { return _d->device_id; }

CANPortPtr CanInterface::can_port() { return _d->can_port; }


bool CanInterface::waitQueueEmpty( Microseconds timeout )
{
    if( isCanReadThread() )
    {
        Log::CAN()->critical(" You CAN NOT use the waitAnswer inside msgReceivedCallback. It would cause an infinite lock");
        throw std::runtime_error("You CAN NOT use the waitAnswer inside msgReceivedCallback. It would cause an infinite wait" );
    }

    absl::Condition is_queue_empty (+[](CanInterface::Impl* _d)
    {
        return (_d->can_write_fifo.empty() &&
                _d->last_msg_wait_answer == CanInterface::Impl::DONT_WAIT);
    }, _d );

    bool done = _d->fifo_mutex.AwaitWithTimeout(is_queue_empty, absl::FromChrono( timeout ) );

    return done;
}

int CanInterface::pushMessage( const CanMessage& m, bool push_front )
{
    _d->can_write_fifo.push( m );

    _d->async_can->addImmediateCallback( std::bind( &CanInterface::Impl::trySendMessage, _d ) );
    return 1;
}

void CanInterface::Impl::trySendMessage()
{
    //even if this is thread safe, we need the lock to make happy the
    // condition variable inside waitQueueEmpty
    LockGuard t( fifo_mutex) ;

    if( last_msg_wait_answer != DONT_WAIT  )
    {
        TimePoint now = GetTimeNow();
        //message already sent on the can device but this queue is blocked
        //nothing to do, stop this loop because you need to wait more
        if( now <= queue_wait_abs_timeout )
        {
            return;
        }
        else
        {
            Log::CAN()->warn("timer thread failed notifying a NO_WAIT");
            last_msg_wait_answer = DONT_WAIT;
        }
    }
    //the case _last_msg_need_answer == NO_WAIT is neutral, you don't need to consider it
    while( can_write_fifo.empty() == false )
    {
        CanMessage msg_to_send;
        can_write_fifo.pop( msg_to_send );

        //send the CanMessage on the can device
        if( can_port->send( &msg_to_send ) == 0 ) // if it is succesfull
        {
            last_msg_sent = msg_to_send;

            num_msg_sent++;
            // default
            last_msg_wait_answer = DONT_WAIT;
            if( last_msg_sent.wait_answer == static_cast<uint32_t>(NEED_TO_WAIT_ANSWER) )
            {
                if( last_msg_sent.desired_answer == 0 )
                {
                    throw std::runtime_error("wait_answer =  NEED_TO_WAIT_ANSWER, but you forgot to set desired_answer" );
                }
                last_msg_wait_answer = WAITING_ANSWER;

                static char temp[20];
                sprintf(temp, "COD_ID: 0x%X",last_msg_sent.desired_answer);
                AsyncManager::Callback_t callback = std::bind( &CanInterface::Impl::timeout_callback, this,  temp );

                async_can->setAlarm( timeout_handle, callback, reply_timeout );
                return;  //stop the while loop
            }
        }
        else {
            Log::CAN()->error("failed to send a message... CAN down?" );
            throw std::runtime_error("failed to send a message.");
        }
    }// end while

    // you get here only if the queue is empty
    return;
}

int timeout_counter_to_exit = 0;

void CanInterface::Impl::timeout_callback(const char* msg)
{
    if( last_msg_wait_answer == WAITING_ANSWER )
    {
        Log::CAN()->warn("*** timeout. *** Sender: {}", last_msg_sent );
    }
    else {
        Log::CAN()->debug("harmless timeout" ) ;
    }
    last_msg_wait_answer = DONT_WAIT;

    async_can->addImmediateCallback( std::bind( &CanInterface::Impl::trySendMessage, this  ) );
}


void CanInterface::Impl::msgReceivedCallback_Sync( const CanMessage& m )
{
    async_can->addImmediateCallback( std::bind( &CanInterface::Impl::msgReceivedCallback_Async, this, m  ) );
}

void CanInterface::Impl::msgReceivedCallback_Async( const CanMessage m )
{
    bool recognized = false;
    {
        if( last_msg_sent.cob_id == m.cob_id)
        {
            // loopback!! Ignore
            return;
        }
        last_msg_received = m;
        num_msg_received++;

        for( unsigned i = 0; i < interpreters_list.size(); i++ )
        {
            recognized |= ( interpreters_list[i] )( m );
        }

        if( !recognized && m.getNode() == node_id)
        {
            Log::CAN()->debug("----answer NOT recognized by interpreters.\n{}",  m);
        }

        //---- check if the queue need to be unblocked.
        if( last_msg_wait_answer == WAITING_ANSWER &&  last_msg_sent.desired_answer == m.cob_id )
        {
            // Log::CAN()->debug() << "cancel timer" ;
            async_can->delAlarm( timeout_handle );
            last_msg_wait_answer = DONT_WAIT;
        }
    }

    async_can->addImmediateCallback( std::bind( &CanInterface::Impl::trySendMessage, this ) );

    return;
}

void CanInterface::Impl::AddReadInterpreter_Async(InterpreterCallback call)
{
    interpreters_list.push_back(call);
}

void CanInterface::addReadInterpreter(InterpreterCallback call)
{
    _d->async_can->addImmediateCallback( std::bind( &CanInterface::Impl::AddReadInterpreter_Async, _d, call  ) );
}

} /* namespace CanMoveIt */
