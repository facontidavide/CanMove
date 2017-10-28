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

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <vector>
#include "cmi/CAN.h"
#include "OS/os_abstraction.h"
#include "cmi/ObjectDatabase.h"
#include "OS/AsyncManager.h"

namespace CanMoveIt{

#if defined (WIN32)
const int PRIORITY_TIMER     = Thread::PRIO_HIGHEST;
const int PRIORITY_CAN_READ  = Thread::PRIO_HIGHEST;
const int PRIORITY_TASK_LOOP = Thread::PRIO_HIGHEST;
#else
const int PRIORITY_TIMER     = 50;
const int PRIORITY_CAN_READ  = 45;
const int PRIORITY_TASK_LOOP = 40;
#endif

// The purpose of this class is to store all the globals of the CMI library.
// We wrapped them inside this class because we want to delete them in a specific order
// in the destructor.

class CMI
{
private:
    CMI();

public:
    static CMI& get()
    {
        static CMI instance;
        return instance;
    }
    // Note: we can't share the same thread with the AsyncManager of
    // EventDistacher. In fact, since we do not know which callbacks the user
    // will subscribe to EventDistacher, we don't want CanInterface to be affected.
    AsyncManager                                async_can;
    AsyncManager                                async_event;

    std::vector< CANPortPtr >                   opened_can_ports;
    std::map<std::string, ObjectsDictionaryPtr> object_dictionaries;

    ~CMI();
};


}

#endif /* GLOBALS_H_ */
