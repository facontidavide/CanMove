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

#include "cmi/globals.h"
#include "cmi/log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

namespace CanMoveIt
{

CMI::CMI():
    async_can(Thread::PRIO_NORMAL),
    async_event(Thread::PRIO_NORMAL)
{
    // Check that only once instance of a CMI controller is running.
#ifdef LINUX
    int pid_file = open("/var/run/canmoveit.pid", O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);
    if(rc) {
        if(EWOULDBLOCK == errno)
        {
            Log::SYS()->critical("Another CanMoveIt controller is running already");
            exit(1);
        }
    }
    else {
        char temp[10];
        pid_t my_pid = getpid();
        Log::SYS()->info( "Starting a process with PID {} (current lock: /var/run/canmoveit.pid)", my_pid);
        int S = sprintf(temp, "%d   ", my_pid );
        write (pid_file, temp, S );
    }
#endif
}

CMI::~CMI()
{
    // The order of these operations IS important, don't change it.
    async_can.kill();
    async_event.kill();

    for( CANPortPtr can_port_handle: opened_can_ports )
    {
        can_port_handle->close( );
    }

    remove("/var/run/canmoveit.pid");

    Log::CAN()->debug("closing");
    Log::CO301()->debug("closing");
    Log::SYS()->debug("closing");
    Log::MAL()->debug("closing");
}


}
