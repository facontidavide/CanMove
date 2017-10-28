/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/


#include <stdio.h>
#include <cassert>
#include <stdlib.h>
#include <deque>
#include <boost/circular_buffer.hpp>
#include "absl/types/any.h"

#include "cmi/CAN.h"
#include "OS/os_abstraction.h"
#include "cmi/globals.h"
#include "cmi/CAN_driver.h"
#include "OS/SharedLibrary.h"

namespace CanMoveIt
{
SharedLibrary CAN_driver_shared_library;
Mutex _can_driver_mutex_;

std::function<int ( CAN_Handle_t, CanMessage* )> canReceive_driver;
std::function<int ( CAN_Handle_t, CanMessage const* )> canSend_driver;
std::function<CAN_Handle_t( const char*, const char* )> canOpen_driver;
std::function<int ( CAN_Handle_t )> canClose_driver;
std::function<int ( CAN_Handle_t, char* )> canChangeBitRate_driver;
std::function<int ( CAN_Handle_t )> canStatus_driver;


class CANPort::Impl{
public:
    int16_t receive(CanMessage *m);
    void receiveLoop();
    void dispatchMessage(const CanMessage& m );
    void tracePush(const CanMessage& m);

    CAN_Handle_t   handle;
    std::string    busname;
    bool           opened;
    Mutex          dispatcher_mutex;

    boost::circular_buffer<CanMessage> trace_queue;
    Mutex                  trace_mutex;
    Condition              trace_cond;
    bool                   trace_enabled;

    typedef struct
    {
        uint16_t       mask;
        uint16_t       frame_id;
        CanRcvCallback callback;
    } SubscriptionInfo;

    std::map<int, SubscriptionInfo> subscribers;
    ThreadPtr                       receive_task;  /**< CAN Receiver task*/

    Impl():
        opened(0),
        trace_queue(50),
        trace_enabled(false)
    {
        handle.fd = -1;
        handle.vp = NULL;
    }
};
//--------------------------------------------------------------

CANPort::CANPort():  _d(new Impl)
{

}

CANPort::~CANPort()
{
    delete _d;
}

void CANPort::traceSetSize(uint32_t s) { _d->trace_queue.resize(s);  }

void CANPort::traceEnable(bool enable)
{
    LockGuard t(_d->trace_mutex);
    _d->trace_enabled = enable;
}

void CANPort::Impl::tracePush(const CanMessage &m)
{
    LockGuard t(trace_mutex);
    if( trace_enabled)
    {
        trace_queue.push_back( m );
        printf("--trace push\n");
    }
}

bool CANPort::tracePop(CanMessage *m, Microseconds timeout)
{
    LockGuard lock( _d->trace_mutex );

    if ( _d->trace_queue.empty() )
    {
        // wait for someone to push
        if (_d->trace_cond.WaitWithTimeout(&_d->trace_mutex, absl::FromChrono(timeout) ) )
        {
            // no timeout: pop.
            *m = _d->trace_queue.front();
            _d->trace_queue.pop_front();
            return true;
        }
        else{ // timeout
            return false;
        }
    }
    else{ // it was not empty.
        *m = _d->trace_queue.front();
        _d->trace_queue.pop_front();
    }
    return true;
}

const char* CANPort::busname() { return _d->busname.c_str(); }

bool CANPort::is_open( ) const { return _d->opened; }


absl::any CANPort::subscribeCallback( CanRcvCallback callback,uint16_t mask, uint16_t frame_id )
{
    static int unique_id = 0;

    LockGuard t(_d->dispatcher_mutex);

    unique_id++;
    Impl::SubscriptionInfo info;
    info.callback = callback;
    info.mask     = mask;
    info.frame_id = frame_id;
    _d->subscribers.insert( std::make_pair( unique_id, info ) );

    return (unique_id);
}

void CANPort::unsubscribeCallback(const absl::any& cb_id )
{
    LockGuard t(_d->dispatcher_mutex);
    int key =  absl::any_cast<int>(cb_id);
    _d->subscribers.erase( _d->subscribers.find(key) );
}

void CANPort::Impl::dispatchMessage(const CanMessage& m )
{
    LockGuard t( dispatcher_mutex );

    Log::CAN()->debug("recv: {}", m );

    std::map<int,SubscriptionInfo>::iterator it;
    for( it = subscribers.begin(); it != subscribers.end(); it++ )
    {
        Impl::SubscriptionInfo* sub = & (it->second);
        uint16_t cobid = m.cob_id;

        if( (sub->frame_id == (cobid &  sub->mask ))  && sub->callback )
        {
            sub->callback( m );
        }
    }
}

int16_t CANPort::Impl::receive(CanMessage *m)
{
    return canReceive_driver( handle, m);
}


bool isCanReadThread()
{
    Thread::ID my_tid = Thread::currentTid();
    for( auto& canport: CMI::get().opened_can_ports )
    {
        if( canport->_d->receive_task->tid() ==  my_tid ) { return true; }
    }
    return false;
}

int16_t CANPort::send( CanMessage* m )
{
    assert( canOpen_driver != NULL );

    m->timestamp_usec = std::chrono::duration_cast<Microseconds>( GetTimeNow().time_since_epoch() ).count();

    int err = canSend_driver( _d->handle, m );
    if (err != 0)
    {
        printf("canSend_driver returnd %d\n",err);
        return err;
       // throw std::runtime_error ("canSend failed");
    }
    m->received = false;
    m->sent     = true;

    //TO TEST async_service.addImmediateCallback( std::bind( &CANPort::Impl::tracePush, _d, *m ) );
     _d->tracePush( *m );

    Log::CAN()->debug("sent: {}", *m);

    return err;
}

void CANPort::Impl::receiveLoop()
{
    CanMessage m;

    Log::CAN()->info("canReceiveLoop started");
    Thread::setCurrentPriority( PRIORITY_CAN_READ );

    while( opened )
    {
        int rc = this->receive(&m);
        if( rc < 0 && opened )
        {
            throw std::runtime_error("Error with CAN receive");
        }
        if( rc == 0)
        {
            m.timestamp_usec = std::chrono::duration_cast<Microseconds>( GetTimeNow().time_since_epoch() ).count();
            m.received = true;
            m.sent     = false;

            // TO TEST async_service.addImmediateCallback( std::bind( &CANPort::Impl::tracePush, this, m ) );
            // TO TEST async_service.addImmediateCallback( std::bind( &CANPort::Impl::dispatchMessage, this, m ) );

             tracePush( m );
             this->dispatchMessage( m );
        }
        // Note if rc == 1 it is a timeout. No dispatching and no throw

#ifdef __KERNEL__
#ifdef USE_XENO
        /* periodic task for Xenomai kernel realtime */
        rt_task_wait_period( NULL );
#endif
#endif
        if( opened == false ) break;
    }
}


CANPortPtr openCanPort(const char* busname, const char* bitrate )
{
    if ( !canOpen_driver )
    {
        throw std::runtime_error("You must load a driver using LoadCanDriver");
    }

    for( unsigned i = 0; i < CMI::get().opened_can_ports.size(); i++ )
    {
        //check if already opened
        if( strcmp( CMI::get().opened_can_ports[i]->busname(), busname ) == 0 )
        {
            return CMI::get().opened_can_ports[i];
        }
    }

    // let's open a new port
    CANPortPtr new_port( new CANPort );
    if( new_port->open( busname, bitrate ) == 0)
    {
        CMI::get().opened_can_ports.push_back( new_port );
    }
    else{
        throw std::runtime_error("Problems opening the CAN port");
    }
    return new_port;
}


int16_t CANPort::open( const char* busname, const char* bitrate )
{
    CAN_Handle_t handle = canOpen_driver( busname, bitrate );
    if( handle.fd != -1 )
    {
        _d->handle  = handle;
        _d->opened  = true;
        _d->busname.assign( busname );
        _d->receive_task = std::make_shared<Thread>();
        _d->receive_task->start( std::bind( &CANPort::Impl::receiveLoop, _d ) );
        this->status( );
        return 0;
    }
    else{
        Log::CAN()->error("CanOpen : Cannot open board. Busname= {}, bitrate= {}", busname, bitrate) ;
        throw std::runtime_error("CanOpen failed");
    }
}

int16_t CANPort::close( )
{
    uint8_t res=0;
    {
        LockGuard t( _can_driver_mutex_);
        _d->opened = false;
        // close CAN port
        res = canClose_driver( _d->handle );
    }
    _d->receive_task->join();

    return res;
}

int16_t CANPort::status( )
{
    int ret = canStatus_driver( _d->handle );
    Log::CAN()->info("CAN STATUS: {}", ret );
    return ret;
}

/*UnLoads the dll*/
int8_t UnLoadCanDriver( )
{
    if( CAN_driver_shared_library.isLoaded() )
    {
        CAN_driver_shared_library.unload();
        return 0;
    }
    return -1;
}

/**
 * Loads the dll and get funcs ptr
 *
 * @param driver_name String containing driver's dynamic library name
 * @return Library handle
 */

template<class T>
std::function<T> loadSymbol( std::string const& functionName )
{
    void* result = CAN_driver_shared_library.getSymbol( functionName );
    return reinterpret_cast<T*>( result );
}


void LoadCanDriver( const char* driver_name )
{
    static bool first_load = true;

    if( first_load )
    {
        CAN_driver_shared_library.load( driver_name );
        first_load = false;
    }

    Log::CAN()->info("Successfully opened {}", driver_name);

    /*Get function ptr*/
    canOpen_driver    = loadSymbol< CAN_Handle_t( const char*, const char* )>("canOpen_driver" );
    canClose_driver   = loadSymbol< int( CAN_Handle_t )>("canClose_driver" );
    canSend_driver    = loadSymbol< int( CAN_Handle_t, CanMessage const* )>("canSend_driver" );
    canReceive_driver = loadSymbol< int( CAN_Handle_t, CanMessage* )>("canReceive_driver" );
    canStatus_driver  = loadSymbol< int( CAN_Handle_t )>("canStatus_driver" );
}


}

