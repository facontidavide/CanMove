#include "OS/Thread.h"
#include <sstream>
#include <stdio.h>
#include "cmi/log.h"

namespace CanMoveIt {

const int Thread::PRIO_LOWEST  = 1;
const int Thread::PRIO_NORMAL  = 40;
const int Thread::PRIO_HIGHEST = 80;
const int Thread::DEFAULT_POLICY = SCHED_OTHER;

static std::string makeName()
{
    static int counter = 1;
    return std::string("thread#") + std::to_string(counter++);
}

Thread::Thread():
    _name(makeName()),
    _priority(PRIO_NORMAL),
    _policy(DEFAULT_POLICY)
{
}


Thread::Thread(std::string name):
    _name( std::move(name) ),
    _priority(PRIO_NORMAL),
    _policy(DEFAULT_POLICY)
{

}

Thread::~Thread()
{}

void Thread::setCurrentPriority(int prio,int  policy )
{
    struct sched_param par;
    par.sched_priority = prio;
    int ret = pthread_setschedparam( pthread_self(), policy, &par);
    if (ret)
    {
        Log::SYS()->error("Warning: Cannot change the thread priority. Code %d\n", ret);
    }
}


void Thread::setPriority(int prio,int  policy )
{
    if (prio != _priority || policy != _policy)
	{
		if ( _thread != NULL )
		{
			struct sched_param par;
			par.sched_priority = prio;
			if (pthread_setschedparam( _thread->native_handle(), policy, &par))
				throw std::runtime_error("cannot set thread priority");
		}
        else  throw std::runtime_error("Thread:: null pointer");
	}
}


} // namespace CanMoveIt
