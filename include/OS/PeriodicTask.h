
#ifndef CMI_PERIODIC_TASK_HPP
#define CMI_PERIODIC_TASK_HPP

#include "OS/Thread.h"

using namespace CanMoveIt;

namespace CanMoveIt {

/** @ingroup os_abstraction
 * This is just an helper abstract class that can be used to manage Periodic threads in an object oriented way.
 * To use it you should:
 * - Create a derived class that inherits from PeriodicTask.
 * - Overload the method startHook (called at the beginning).
 * - Overload updateHook that is called periodically.
 * - Overload, if needed, stopHook, that will be executed once thetask is completed.
 **/
class PeriodicTask
{

public:
    PeriodicTask();

    ~PeriodicTask();


    /** Launch the periodic thread.
    * @param period period in microseconds
    * @param priority optional: set the priority of the thread.
    **/
    void start_execution(Microseconds period, int priority = Thread::PRIO_NORMAL);

    /** Stop the eperiodic thread and destroy it.
     **/
    void stop_execution();

    /** This is a blocking function that waits the end of the thread related
     * to the periodic task.
     * Most of the times it is used to prevent that your main() reach the end.
     * Optionally, you can add a timeout (0 means that no timeout is used at all).
     **/
    void wait_completion(Milliseconds timeout = Milliseconds(0));

    /** You should overload this method with your own implementation.
    * It is called once at the beginning.
    **/
    virtual void startHook()=0;

    /** You should overload this method with your own implementation.
    * It is called periodically. The first invocation takes place one period time after startHook().
    **/
    virtual void updateHook() =0;

    /** You should overload this method with your own implementation.
    * It is called once, just after stop_execution and before the thread is killed.
    **/
    virtual void stopHook() =0;

    /// Get the period in microseconds
    Microseconds getPeriod();

    /// Set the period in microseconds
    void setPeriod(Microseconds period);


private:
    void run(void*);

    class Impl;
    Impl* _d;
};


} /* namespace CanMoveIt */

#endif
