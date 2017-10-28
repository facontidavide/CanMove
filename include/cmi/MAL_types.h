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
#ifndef MOTORHAL_TYPES_H
#define MOTORHAL_TYPES_H
#include "builtin_types.hpp"

namespace CanMoveIt {

/// @ingroup MAL
/// Flags that can be passed to the method MAL_Interface::setProfiledPositionTarget
typedef enum {
    PROFILE_CHANGE_IMMEDIATELY = 1 << 5,
    PROFILE_RELATIVE_POS       = 1 << 6,
    PROFILE_HALT               = 1 << 8,
    PROFILE_BUFFERED_POINT     = 1 << 13
}ProfiledPositionFlags;


/// @ingroup MAL
/// Possible return values when a command is send using the MAL_Interface.
typedef enum {
    SUCCESSFUL = 0,
    ERROR_NOT_READY = 2,
    ERROR_VALUE_OUT_OF_RANGE  = 3,
    ERROR_WRONG_MODE = 4,
    ERROR_NOT_AVAILABLE = 6,
    ERROR_WRONG_PARAMETER = 7,
    ERROR_NOT_CONNECTED = 8,
    ERROR_TIMEOUT = 10
}CommandResult;

/// @ingroup MAL
/// Supported types of fault.
typedef enum
{
    NO_FAULT = 0x0,
    FAULT_OVER_CURRENT = 0x1,
    FAULT_OVER_VOLTAGE = 0x2,
    FAULT_UNDER_VOLTAGE = 0x4,
    FAULT_FOLLOWING_ERROR = 0x8,
    FAULT_OVER_TEMPERATURE = 0x10,
    FAULT_IP_BUFFER_UNDERFLOW = 0x20

}FaultType;

const int ALL_MOTORS = 0xFFF;

/// @ingroup MAL
/// Current status of the servodrive/motor.
typedef enum{
    FAULT_STATE = 0,
    NOT_READY = 1,
    SWITCH_ON_DISABLED = 2,
    READY_TO_SWITCH_ON = 3,
    SWITCHED_ON = 4,
    OPERATION_ENABLED = 5,
    QUICK_STOP_ACTIVE = 6,
    FAULT_REACTION=7,
    STATUS_NOT_INITIALIZED = 255
}MotorStatus;

/// @ingroup MAL
/// Helper function to convert MotorStatus to string.
///
inline const char* StatusToString(MotorStatus num)
{
    if( num == OPERATION_ENABLED )      return "OPERATION_ENABLED";
    if( num == QUICK_STOP_ACTIVE )      return "QUICK_STOP_ACTIVE";
    if( num == FAULT_STATE )            return "FAULT_STATE";
    if( num == SWITCHED_ON )            return "SWITCHED_ON";
    if( num == STATUS_NOT_INITIALIZED ) return "STATUS_NOT_INITIALIZED";
    if( num == NOT_READY )              return "NOT_READY";
    if( num == SWITCH_ON_DISABLED )     return "SWITCH_ON_DISABLED";
    if( num == FAULT_REACTION )         return "FAULT_REACTION";
    if( num == READY_TO_SWITCH_ON)      return "READY_TO_SWITCH_ON";
    return "not_recognized";
}


/// @ingroup MAL
typedef enum{
    /** Profiled position (also known as "trapezoidal interpolation" is a mode
      where the user send a target position, a maximum velocity and maximum acceleration.
      The servo drives use these three values to create a smooth path from the current position
      to the target one.
    */
    PROFILED_POSITION_MODE = 1,

    /** Velocity control. Similarly to the profiled position, maximum acceleration should be defined
      to guarantee smooth velocity transitions. */
    PROFILED_VELOCITY_MODE = 3,

    /** The torque mode should actually be called Current Mode, since it is the current of the
      motor to be controlled. You can use the Nm/Amps constant of the motor windings to obtain the
      torque. Consider that the actual torque at the joint is influenced by other factors such as
      friction and gearbox reduction ration and efficiency. */
    TORQUE_MODE = 4,

    /** The interpolated position is used when the user's application takes care of the interpolation
      of the motor position. In this case a stream of traget positions, equally spaced in time, is provided
      by the client. You should be careful about the problem of buffer underflow. */
    INTERPOLATED_POSITION_MODE = 7,

    /** Value used to initialize empty variables. */
    UNDEFINED_MODE = 0
} ModeOperation;

/** @ingroup MAL
 * Helper function to convert ModeOperation to string
*/
inline const char* ControlModeToString(ModeOperation num)
{
    if( num == PROFILED_POSITION_MODE )      return "PROFILED_POSITION_MODE";
    if( num == PROFILED_VELOCITY_MODE )      return "PROFILED_VELOCITY_MODE";
    if( num == TORQUE_MODE )                 return "TORQUE_MODE";
    if( num == INTERPOLATED_POSITION_MODE )  return "INTERPOLATED_POSITION_MODE";
    return "not_recognized";
}

/// @ingroup MAL
typedef enum {
    /** Velocity between the acceleration and deceleration phases. */
    PROFILE_VELOCITY = 1,
    /** Acceleration phase of the trapezoidal interpolator. */
    PROFILE_ACCELERATION = 2,
    /** Deceleration phase of the trapezoidal interpolator. */
    PROFILE_DECELERATION = 3,
}ProfileParameter;


}

#endif // MOTORHAL_TYPES_H
