#ifndef CMI_Thread_INCLUDED
#define CMI_Thread_INCLUDED

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "OS/Time.h"
#include "absl/synchronization/mutex.h"

namespace CanMoveIt {

using Mutex      = absl::Mutex;

class LockGuard: absl::MutexLock
{
public:
    LockGuard( Mutex& m ):  absl::MutexLock(&m) {}
    ~LockGuard() = default;
};

// check if it works even if it not recusrsive
using RecursiveMutex     = absl::Mutex;
using RecursiveLockGuard = LockGuard;

using Condition  = absl::CondVar;

using RW_Mutex         = absl::Mutex;
using ScopedReadLock   = absl::ReaderMutexLock;
using ScopedWriteLock  = absl::WriterMutexLock;

void prepare_rt_platform();


        /// This class implements a platform-independent
        /// wrapper to an operating system thread.
        ///
        /// Every Thread object gets a unique (within
        /// its process) numeric thread ID.
        /// Furthermore, a thread can be assigned a name.
        /// The name of a thread can be changed at any time.
class Thread
{

private:

    std::string    _name;
    mutable Mutex  _mutex;

    std::shared_ptr<std::thread> _thread;
//    std::thread::attributes        _attributes;
    int _priority;
    int _policy;

    /// Threads are NOT copiable
    Thread& operator = (const Thread&);

    /// Threads are NOT copiable
    Thread(const Thread&);

public:	

    static const int PRIO_LOWEST;
    static const int PRIO_HIGHEST;
    static const int PRIO_NORMAL;
    static const int DEFAULT_POLICY;

    typedef std::thread::id ID;

    typedef std::function<void(void*)> Callable_t;

    /// Creates a thread. Call start() to start it.
    Thread();


    /// Creates a named thread. Call start() to start it.
    Thread(std::string name);

    /// Destroys the thread.
    ~Thread();

    /// Returns the native thread ID of the thread.
    ID tid() const;

    /// Returns the name of the thread.
    const char* getName() const;

    /// Sets the name of the thread.
    void setName(const char* name);

    /// Sets the thread's priority, using an operating system specific
    /// priority value. Use getMinPriority() and getMaxPriority() to
    /// obtain mininum and maximum priority values. Additionally,
    /// a scheduling policy can be specified. The policy is currently
    /// only used on POSIX platforms where the values SCHED_OTHER (default),
    /// SCHED_FIFO and SCHED_RR are supported.
    void setPriority(int prio, int policy = DEFAULT_POLICY);

    /// Returns the thread's priority, expressed as an operating system
    /// specific priority value.
    ///
    /// May return 0 if the priority has not been explicitly set.
    int getPriority() const;

    /// Returns the mininum operating system-specific priority value,
    /// which can be passed to setPriority() for the given policy.
    static int getMinPriority(int policy = DEFAULT_POLICY);

    /// Returns the maximum operating system-specific priority value,
    /// which can be passed to setPriority() for the given policy.
    static int getMaxPriority(int policy = DEFAULT_POLICY);

    /// Sets the thread's stack size in bytes.
    /// Setting the stack size to 0 will use the default stack size.
    /// Typically, the real stack size is rounded up to the nearest
    /// page size multiple.
    void setStackSize(int size);

    /// Returns the thread's stack size in bytes.
    /// If the default stack size is used, 0 is returned.
    int getStackSize() const;

    /// Starts the thread with the given target and parameter.
    void start(Callable_t target, void* pData = 0);

    /// Waits until the thread completes execution.
    /// If multiple threads try to join the same
    /// thread, the result is undefined.
    void join();

    /// Returns true if the thread is running.
    bool isRunning() const;

    /// Yields cpu to other threads.
    static void yield();

    /// Returns the native thread ID for the current thread.
    static ID currentTid();

    /// Returns the native thread ID for the current thread.
    static void setCurrentPriority(int prio,int policy = DEFAULT_POLICY );

protected:


};

typedef std::shared_ptr<Thread> ThreadPtr;

//
// inlines
//
inline Thread::ID Thread::tid() const
{
    if( _thread == NULL )  throw std::runtime_error("Thread: null pointer");
    return _thread->get_id();
}


inline const char* Thread::getName() const
{
    return _name.c_str() ;
}

inline bool Thread::isRunning() const
{
    if( _thread == NULL )  throw std::runtime_error("Thread: null pointer");
    return _thread->joinable();
}

inline void Thread::yield()
{
    std::this_thread::yield();
}


inline int Thread::getPriority() const
{
    return _priority;
}

inline int Thread::getMinPriority(int /*policy*/)
{
    return PRIO_LOWEST;
}


inline int Thread::getMaxPriority(int /*policy*/)
{
    return PRIO_HIGHEST;
}


inline Thread::ID Thread::currentTid()
{
    return std::this_thread::get_id();
}

inline void Thread::start(Callable_t target, void* data_ptr )
{
    _thread = std::make_shared<std::thread>( std::bind( target, data_ptr ) );
}

inline void Thread::join()
{
    if(_thread == NULL ) throw std::runtime_error("Thread: null pointer");
    if(_thread->joinable()) _thread->join();
}


inline void Thread::setName(const char *name)
{
    absl::MutexLock lock(&_mutex);
    _name = name;
}

} // namespace CanMoveIt


#endif // CMI_Thread_INCLUDED
