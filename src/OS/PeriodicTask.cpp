#include <stdio.h>
#include <signal.h>
#include "OS/PeriodicTask.h"
#include "cmi/log.h"

namespace CanMoveIt{

bool _cmi_KILL_SIGNAL_RECEIVED = false;

class PeriodicTask::Impl
{
public:
    CanMoveIt::Thread thread;
    Microseconds      period;
    int               priority;
    bool              running;
};

// Define the function to be called when ctrl-c (SIGINT) signal is sent to process
/*void signal_callback_handler(int signum)
{
    printf("Caught signal %d\n",signum);
    // Cleanup and close up stuff here
    _cmi_KILL_SIGNAL_RECEIVED =true;
}*/

Microseconds PeriodicTask::getPeriod() {return _d->period;}


void PeriodicTask::setPeriod(Microseconds period) {_d->period = period; }

PeriodicTask::PeriodicTask(): _d(new Impl)
{
    _d->running = false;
}

PeriodicTask::~PeriodicTask() { delete _d; }

void PeriodicTask::run(void*)
{
    // Register signal and signal handler
   // signal(SIGINT, signal_callback_handler);

    printf("PeriodicTask: ");
    Thread::setCurrentPriority( _d->priority );

    this->startHook();

    TimePoint deadline = GetTimeNow();

    while( _d->running && !_cmi_KILL_SIGNAL_RECEIVED)
    {
        // first, sleep until the deadline is reached
        TimePoint now = GetTimeNow();
        int overrun = -1;

        do{
            deadline += _d->period;
            overrun++;
        }while ( deadline < now);

      //  if (overrun) printf("warning: overrun of %d cycles in periodic task\n",overrun);

        CanMoveIt::sleepUntil(deadline);

        TimePoint t1 = GetTimeNow();
        //---------------
        updateHook();
        //---------------
        TimePoint t2 = GetTimeNow();
		Microseconds diff = duration_cast<Microseconds>(t2-t1);
        if( diff >  _d->period)
        {
            Log::SYS()->warn("you updateHook took more than {} usec\n", (long)_d->period.count() );
        }
    }
    stopHook();

    printf("end of periodic task\n");

}

void PeriodicTask::start_execution(Microseconds period, int priority)
{
    // these must go before the creation of the thread to avoid data race
    _d->running = true;
    _d->period = period ;
    _d->priority = priority;
    // start the thread
    _d->thread.start( std::bind( &PeriodicTask::run, this, std::placeholders::_1 ) , NULL );
}

void PeriodicTask::stop_execution()
{
    _d->running = false;
}

void PeriodicTask::wait_completion(Milliseconds timeout)
{
    if( timeout != Milliseconds(0) )
    {
        //TODO _d->thread.tryJoin(timeout);
    }
    else{
        _d->thread.join();
    }
}

}//end namespace CanMOveIt
