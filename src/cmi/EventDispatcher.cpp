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

#include "cmi/EventDispatcher.h"
#include "cmi/globals.h"
#include "OS/AsyncManager.h"
#include <boost/circular_buffer.hpp>
#include <boost/serialization/strong_typedef.hpp>
#include <boost/asio.hpp>

namespace CanMoveIt{


void theEndOfTime(const boost::system::error_code& ) {}


void ExecuteSynchronousCallback_E(EventCallback callback, uint16_t device_id, EventData const& data )
{
    if(callback)
        callback(device_id, data);
}



void _default_display_callback(uint16_t device_id, ObjectEntry const& entry, ObjectData const&  data)
{
    std::cout << std::hex <<  "Event ( 0x" << entry.index()
              << " / 0x" << entry.subindex() << " ):  "
              <<  data << std::endl;
}

//--------------------------------------------------------------------------
struct EventDispatcher::Impl
{
    typedef struct
    {
        EventRepeat     enabled;
        EventMode       mode;
        EventCallback   callback;
    }EventInfo;


    typedef std::multimap<uint32_t, EventInfo>::iterator map_iterator;
    std::deque< std::function<void(void)> >      callback_fifo;

    std::multimap<EventID, EventInfo>     info_list;
    RecursiveMutex                        mutex;
};


EventDispatcher::EventDispatcher(): _d(new Impl)
{

}

EventDispatcher::~EventDispatcher()
{
    delete _d;
}


EventPtr EventDispatcher::add_subscription(EventID const&  id, EventMode const& mode, EventCallback callback)
{
    RecursiveLockGuard lock( _d->mutex );
    Impl::EventInfo info;
    info.mode = mode;
    info.enabled = EVENT_ENABLED;

    if( !callback && ( mode == CALLBACK_ASYNCH || mode ==  CALLBACK_SYNCH) )
    {
        throw std::runtime_error("callback can't be null");
    }
    info.callback = callback;

    Impl::map_iterator it = _d->info_list.insert( std::make_pair(id, info));
    return it;
}

void EventDispatcher::configureMode(EventPtr event,EventMode mode)
{
    RecursiveLockGuard lock( _d->mutex );
    Impl::map_iterator it = absl::any_cast<Impl::map_iterator>( event );
    it->second.mode = mode;
}

void EventDispatcher::configureCallback(EventPtr event,EventCallback callback)
{
    RecursiveLockGuard lock( _d->mutex );
    Impl::map_iterator it = absl::any_cast<Impl::map_iterator>( event );
    it->second.callback = callback;
}

//--------------------------------------------------------
void EventDispatcher::configureRepeat(EventPtr event, EventRepeat r)
{
    RecursiveLockGuard lock( _d->mutex );
    Impl::map_iterator it = absl::any_cast<Impl::map_iterator>( event );
    it->second.enabled = r;
}
//--------------------------------------------------------
void EventDispatcher::eraseEvent(EventPtr event)
{
    RecursiveLockGuard lock( _d->mutex );
    Impl::map_iterator it = absl::any_cast<Impl::map_iterator>( event );
    _d->info_list.erase( it );
}

void EventDispatcher::eraseEvents(EventID const& event_id )
{
    RecursiveLockGuard lock( _d->mutex );

    for(auto it = _d->info_list.begin(); it != _d->info_list.end(); )
    {
        if(it->first == event_id)
            it = _d->info_list.erase(it);
        else
            ++it;
    }
}


//--------------------------------------------------------

void EventDispatcher::push_event(uint16_t device_id, EventData const& data)
{
    RecursiveLockGuard lock( _d->mutex  );

    std::pair<Impl::map_iterator,Impl::map_iterator> it_range = _d->info_list.equal_range( data.event_id );

    for (Impl::map_iterator it = it_range.first; it != it_range.second; it++ )
    {
        Impl::EventInfo *event_info = &( it->second );

        if( event_info->enabled != EVENT_DISABLED )
        {
            EventMode mode = event_info->mode;

            switch (mode)
            {
                case PRINT_ON_STREAM:{ data.print(); } break;

                case CALLBACK_ASYNCH:{
                    _d->callback_fifo.push_back( std::bind( event_info->callback, device_id, data ) );
                }break;

                case CALLBACK_SYNCH_CANREAD:{
                    event_info->callback(device_id, data );
                }break;

                case CALLBACK_SYNCH:{

                    // Note: it could be confusing that we use an AsynManager behind what
                    // we called CALLBACK_SYNCH.
                    // But this manager has its own thread that activaly and continuously calls
                    // run independently from the user's thread; for this reason we call it
                    // "Synchronous".
                    CMI::get().async_event.addImmediateCallback(
                                std::bind( &ExecuteSynchronousCallback_E,
                                           event_info->callback,
                                           device_id,
                                           data)
                                );
                }break;
            }
        }
        if( event_info->enabled == EVENT_ENABLE_ONCE) event_info->enabled = EVENT_DISABLED;
    }
}

void EventDispatcher::spin(Microseconds ms)
{
    absl::Condition is_fifo_empty( +[](EventDispatcher::Impl* d)
    {
        return d->callback_fifo.empty();
    }, _d );

    bool done = _d->mutex.AwaitWithTimeout(is_fifo_empty, absl::FromChrono(ms) );

    if( !done )
    {
        return;
    }

    while ( _d->callback_fifo.size() > 0)
    {
        // execute the callback.
        _d->callback_fifo.front() ();
        _d->callback_fifo.pop_front();
    }
}



} /* namespace CanMoveIt */

