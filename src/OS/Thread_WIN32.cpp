#include "OS/Thread.h"
#include "OS/Mutex.h"
#include "OS/Exception.h"
#include <sstream>
#include <windows.h>

namespace CanMoveIt {

const int Thread::PRIO_LOWEST  = THREAD_PRIORITY_LOWEST;
const int Thread::PRIO_NORMAL  = THREAD_PRIORITY_NORMAL;
const int Thread::PRIO_HIGHEST = THREAD_PRIORITY_HIGHEST;

#ifdef WIN32
const int Thread::DEFAULT_POLICY = 0;
#else
const int Thread::DEFAULT_POLICY = SCHED_FIFO;
#endif

Thread::Thread():
    _name(makeName()),
    _priority(PRIO_NORMAL)
{
}


Thread::Thread(const std::string& name):
    _name(name),
    _priority(PRIO_NORMAL)
{
}


Thread::~Thread()
{

}

void Thread::setCurrentPriority(int prio,int /* policy */)
{
    if (SetThreadPriority( GetCurrentThread() , prio) == 0)
                throw std::runtime_error("cannot set thread priority");
}


void Thread::setPriority(int prio,int /* policy */)
{
    if (prio != _priority)
    {
        _priority = prio;
        if (_thread != NULL)
        {
            if (SetThreadPriority( _thread->native_handle(), _priority) == 0)
                throw std::runtime_error("cannot set thread priority");
        }
        else  throw NullPointerException();
    }
}


} // namespace CanMoveIt
