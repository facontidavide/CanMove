/*
 * AsyncManager.cpp
 *
 *  Created on: Jan 23, 2013
 *      Author: davide
 */

#include <boost/asio/posix/basic_descriptor.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include "OS/AsyncManager.h"

namespace CanMoveIt {

typedef std::shared_ptr< boost::asio::deadline_timer > DeadLinePtr;

class  AsyncManager::Impl{

public:
    boost::asio::io_service			   io_service;
    boost::asio::strand			       strand;
    boost::asio::deadline_timer        dummy_timer;
    std::shared_ptr<boost::thread>     timer_thread;

    Impl(): strand(io_service), dummy_timer(io_service) {}
};

void AsyncManager_intermediate_callback(const boost::system::error_code& error, AsyncManager::Callback_t actual_callback )
{
    if (error == boost::asio::error::operation_aborted)
    {
        return;
    }

    if( actual_callback != NULL)
    {
        actual_callback();
    }
}

AsyncManager::AsyncManager(uint8_t priority):
    _d( new Impl)
{

    _d->dummy_timer.expires_at( boost::posix_time::max_date_time );
    _d->dummy_timer.async_wait( _d->strand.wrap(
                                    boost::bind( AsyncManager_intermediate_callback,
                                               boost::asio::placeholders::error,
                                               Callback_t() )
                                    )
                                );

    _d->timer_thread = std::shared_ptr<boost::thread>
            (new boost::thread( boost::bind(&boost::asio::io_service::run, &_d->io_service)) );
}


AsyncManager::AsyncManager(NO_OWN_THREAD ):
     _d( new Impl)
{

}


AsyncManager::~AsyncManager()
{
    // cancel all pending timers
    _d->io_service.stop();
    // wait for the spawned thread to complete.
    if( _d->timer_thread)
        _d->timer_thread->join();
    // printf("cleaned up\n");
    delete _d;
}


void AsyncManager::kill()
{
    _d->io_service.stop();

    if( _d->timer_thread)
            _d->timer_thread->join();
}

void AsyncManager::delAlarm(Handle_t alarm)
{

    absl::any_cast<DeadLinePtr>(alarm)->cancel();
}

AsyncManager::Handle_t AsyncManager::addAlarm()
{
    return DeadLinePtr( new boost::asio::deadline_timer(_d->io_service) );
}

void AsyncManager::setAlarm(Handle_t alarm, Callback_t callback, Microseconds timer_value )
{
    DeadLinePtr al = absl::any_cast<DeadLinePtr>(alarm);
    boost::posix_time::microseconds deadline( timer_value.count() );
    al->expires_from_now( deadline );

    al->async_wait( _d->strand.wrap(boost::bind( &AsyncManager_intermediate_callback,
                                                  boost::asio::placeholders::error,
                                                  callback) ) );
}

void AsyncManager::addImmediateCallback(Callback_t callback)
{
    boost::system::error_code error;
    _d->io_service.post( _d->strand.wrap(boost::bind( &AsyncManager_intermediate_callback,
                                                    error,
                                                    callback) ) );
}

void AsyncManager::flush_expired()
{
    _d->io_service.poll();
}


} // namespace CanMoveIt 
