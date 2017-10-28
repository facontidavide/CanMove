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

#ifndef CANOPEN_402_H
#define CANOPEN_402_H

#include "CO301_def.h"
#include "ObjectDictionary.h"

namespace CanMoveIt {

const ObjectID CONTROLWORD ( 0x6040, 0);
const ObjectID STATUSWORD ( 0x6041, 0);
const ObjectID FAULT_DETECTION ( 0x605E, 0);
const ObjectID MODE_OPERATION ( 0x6060, 0);
const ObjectID MODE_OPERATION_DISPLAY ( 0x6061, 0);
const ObjectID POSITION_ACTUAL_VALUE ( 0x6064, 0);
const ObjectID TARGET_TORQUE ( 0x6071, 0);
const ObjectID MAX_TORQUE ( 0x6072, 0);
const ObjectID MAX_CURRENT ( 0x6073, 0);

const ObjectID RATE_CURRENT ( 0x6075, 0);
const ObjectID RATE_TORQUE ( 0x6076, 0);

const ObjectID TORQUE_DEMAND_VALUE ( 0x6074, 0);
const ObjectID FOLLOWING_ERROR_ACTUAL ( 0x60F4, 0);
const ObjectID ERRORCODE ( 0x603F, 0);
const ObjectID CURRENT_ACTUAL_VALUE ( 0x6078, 0);
const ObjectID TARGET_VELOCITY ( 0x60FF, 0);

const ObjectID VELOCITY_ACTUAL_VALUE( 0x606C, 0 );

const ObjectID TARGET_POSITION ( 0x607A, 0);
const ObjectID POSITION_RANGE_LIMIT_MIN ( 0x607B, 1);
const ObjectID POSITION_RANGE_LIMIT_MAX ( 0x607B, 2);
const ObjectID SOFTWARE_POSITION_LIMIT_MIN ( 0x607D, 1);
const ObjectID SOFTWARE_POSITION_LIMIT_MAX ( 0x607D, 2);
const ObjectID MAXIMUM_PROFILE_VELOCITY ( 0x607F, 0);
const ObjectID PROFILED_VELOCITY ( 0x6081, 0);
const ObjectID END_VELOCITY ( 0x6082, 0);
const ObjectID PROFILED_ACCELERATION ( 0x6083, 0);
const ObjectID PROFILED_DECELERATION ( 0x6084, 0);
const ObjectID MOTION_PROFILE_TYPE ( 0x6086, 0);
const ObjectID MAXIMUM_ACCELERATION ( 0x60C5, 0);
const ObjectID MAXIMUM_DECELERATION ( 0x60C6, 0);


const uint16_t PROFILE_TARGET_REACHED     = 1 << 10;
const uint16_t PROFILE_SET_ACKNOWLEDGE    = 1 << 12;
const uint16_t PROFILE_FOLLOWING_ERROR    = 1 << 13;

const uint16_t  INTERPOLATED_DATA_RECORD = 0x60C1;

}



#endif // CANOPEN_H
