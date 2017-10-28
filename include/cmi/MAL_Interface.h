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

#ifndef Abs_Controller_H
#define Abs_Controller_H

#include "cmi/CAN_Interface.h"
#include "cmi/MAL_types.h"
#include "cmi/CO402_def.h"


namespace CanMoveIt{

/// Event type raised when a target position was reached in PROFILED_POSITION_MODE
const EventID EVENT_PROFILED_POSITION_REACHED( 4 );


/** @defgroup MAL L3: Motor Abstract Layer.
 *  @brief This layer implements the Cia 402 profile.
 * Its API focuses on the control of motors and servo drive instead of generic CANopen devices.
 */

/** @ingroup MAL
 *  @class MAL_Interface

 * @brief Abstract class that defines the interface used to control a single motor.
 *
 * This class represents an hardware abstract layer for motors. It hides details such as
 * the protocol, the bus, the mechanical parameters (encoder resolution and gear reduction).
 * It also takes care of initialization and configuration of the servo drives; the particular configuration
 * selected might be considered "arbitrary" by a small amount of expert users which want to configure
 * the drive differently. In this case, they should modify the class configureDrive and, sometimes, startDrive too.
 */
class MAL_Interface{

protected:

    /**
     * Constant to convert from joint to motor position.
	 * It is obtained multiplying encoder resoultion and gearbox reduction.
	 */
    ModeOperation  _mode_operation;
    int32_t          _safety_max_current;
    double         _rad_to_encoder;

public:

    /**
     * @param ID
     * 		An identifier that is passed by the user. It should be unique for each motor.
     * 		It doesn't need to be equal to the node ID of the CAN bus.
     */
    MAL_Interface(int16_t ID, uint32_t max_safety_current = 10000);

    /// Returns the ID of this instance. Not to be confused with the node of the CAN node.
    virtual int16_t getID() const = 0;

    /** Set the rad_to_encoder constant. It takes encoder resolution, direction of motion and gearbox reduction into account. */
    void   setRadToEncoder(double r_to_e)   { _rad_to_encoder = r_to_e;}

    /** Get the rad_to_encoder constant. */
    double getRadToEncoder()                { return _rad_to_encoder; }

    /** set the PID parameters at the level of the servo drive. These parameters are used in the POSITION_MODE.
        @param P - Proportional
        @param I - Integral.
        @param D - Derivative
        @return see CommandResult.
    */
    virtual CommandResult setPID(double P, double I, double D) =0;

    /** This onfiguration step must be done at least ONCE every time the servo drive is switched on.
      In CANopen, in particular, it configures the PDO mapping.
      */
    virtual CommandResult configureDrive(const char *filename = NULL) =0;
    /**
     * Start the drive or restart it if it was in fault state.
     * Note: the implementation of this method is blocking, i.e. it will not return until the drive was
     * started. Potentially this will become non-blocking in the future (or include a timeout)
     *
     * return SUCCESSFUL if we were able to switch to OPERATION_ENABLED or ERROR_NOT_READY otherwise.
     */
    virtual CommandResult startDrive() =0;
    /**
     * Stop motion.
     */
    virtual void stopDrive() =0;

    /** changes the mode of operation.
         @param mode    Use the mode definitions in ModeOperation.
         @param force   By default if you are already in the right mode, no CAN request is sent.
                        If you want to be sure that this action takes place and the related SDO write are
                        executed, then set force = true.
         @return see CommandResult.
    */
    virtual CommandResult setModeOperation(ModeOperation mode,bool force = false) =0;

    /** Set period between one interpolated position and the next.
     * It will work only if INTERPOLATED_POSITION_MODE is enabled.
     @param milliseconds Period in milliseconds.
     @return see CommandResult.
    */
    virtual CommandResult setInterpolatedPositionPeriod(Milliseconds milliseconds) =0;

    /** By default, when you invoke for the first time the command pushInterpolatedPositionTarget,
     * the application will also start tell to the servo drive to start consuming the interpolate position
     * buffer (this is done modifying the controlword with SDO). This is called "autostart".
     * If you want multiple axis to be perfectly synchonized, you might prefer to set to false autostart and
     * to use the command sendInterpolatedPositionStart. */
    virtual void setInterpolatedPositionAutostart(bool autostart) =0;

    /** This command will affect all the drives that were configured with setModeOperation(INTERPOLATED_POSITION)
     *  and setInterpolatedPositionAutostart(false).
     * It will broadcast a control word to all the devices using PDO_RX. */
    virtual void sendInterpolatedPositionStart() =0;

    /** Set the desired target position.
     * It will work only if INTERPOLATED_POSITION_MODE is enabled.
         @param pos_in_rad - desired target position in radians
         @param vel_rad_sec - desired velocity in rad/sc. Used only by PVT interpolators, ignored by PT interpolators.
         @return see CommandResult.
    */
    virtual CommandResult pushInterpolatedPositionTarget(double pos_in_rad, double vel_rad_sec = 0) =0;

    /** Set the target position in PROFILED_POSITION_MODE.
     * By default the position sent is absolute and non buffered. ou can change such mode using
     * the flags PROFILE_RELATIVE_POS and PROFILE_BUFFERED_POINT.
     * If you want a callback to be executed when the target is reached, you should add it
     * using the method setCallbackTargetReached
     */
    virtual CommandResult setProfiledPositionTarget(double pos_in_rad, uint16_t flags=0 ) =0;

    /** Stop the execution of the profiled position*/
    virtual CommandResult haltProfiledPosition() =0;

    /** The mode of the trapezoidal interpolator of the PROFILED_POSITION_MODE
    *  can be changed setting different parameters. See values of the type ProfileParameter for more details.
    */
    virtual CommandResult setProfileParameter(ProfileParameter param, double value) =0;

    virtual void getProfileParameter(ProfileParameter param, double *value) =0;

    /** Set the velocity in PROFILED_VELOCITY_MODE.
     * @param rad_sec desired velocity in radians per second.
     **/
    virtual CommandResult setProfiledVelocityTarget(double rad_sec) =0;

    /** Set the desired torque of the motor, expressed in mAmps. It will only if TORQUE_MODE is enabled.
       @param curr_in_mA - current in mAmps. It is up to you to do the convertion to torque (Nm) considering your motor constants.
   	   @return see CommandResult.
    */
    virtual CommandResult setCurrentTarget(int32_t curr_in_mA) =0;

    /** Maximum current that will be generated by the PID controller.
     * It is not used in TORQUE_MODE, since the current is specified directly by the user.
       @param curr_in_mA - current in mAmps. It is up to you to do the convertion to torque (Nm) considering your motor constants.
       @return see CommandResult.
    */
    virtual CommandResult setCurrentLimit(uint32_t curr_in_mA) =0;

    /// Get the actual value of the motor current limit.
    virtual void getCurrentLimit(uint32_t *curremt_limit) =0;

    /// Get the status of the motor
    virtual  MotorStatus  getStatus() =0;

    /// Use this method to reset a fault on a slave.
    virtual void resetFaults() =0;

    /** Read the last fault from the drive. This operation might be blocking. */
    virtual FaultType getLastFault() =0;

    /** Get the actual mode of operation.
      */
    ModeOperation  getModeOperation()    {return _mode_operation;}

    /** Read the last received value of Position Error.
     * Note: this commands does NOT trigger a request on the bus, it just reads a locally stored value.
     * Check the return value of type DataStatus to see if it is the first time you read it
     */
    virtual DataStatus getPositionError(double* pos_err)=0;

    /** Read the last received value of Actual Position.
     * Note: this commands does NOT trigger a request on the bus, it just reads a locally stored value.
     * Check the return value of type DataStatus to see if it is the first time you read it
     */
    virtual DataStatus getActualPosition(double* act_pos)=0;

    /** Read the last received value of Actual Current.
     * Note: this commands does NOT trigger a request on the bus, it just reads a locally stored value.
     * Check the return value of type DataStatus to see if it is the first time you read it
     */
    virtual DataStatus getActualCurrent(int32_t* act_curr)=0;

    /** Some drivers can be configured to send automatically the actual position.
     * If it is not your case, you should call this method to request this data.
     * The answer will be received asynchonously and can be accessed using getActualPosition.
      */
    virtual CommandResult requestActualPosition()=0;

    /** Some drivers can be configured to send automatically the actual current.
      * If it is not your case, you should call this method to request this data.
     * The answer will be received asynchonously and can be accessed using getActualPosition.
      */
    virtual CommandResult requestActualCurrent()=0;

    /** Some drivers can be configured to send automatically the actual velocity.
      * If it is not your case, you should call this method to request this data.
     * The answer will be received asynchonously and can be accessed using getActualPosition.
      */
    virtual CommandResult requestActualVelocity()=0;

    /** Some drivers can be configured to send automatically the position error.
      * If it is not your case, you should call this method to request this data.
     * The answer will be received asynchonously and can be accessed using getPositionError.
      */
    virtual CommandResult requestPositionError()=0;

    /// Check if the particular implementation is CanOpen or not.
    virtual bool isCanOpen() {return false;}

    /** Interface to the instance of EventDispatcher related to this particular node/slave. */
    virtual EventDispatcher* events() =0;

    virtual DataStatus getActualVelocity(double *vel) = 0;

    virtual bool isMotorReady() =0;

    uint32_t getMaxAllowedCurrent( ) const         { return _safety_max_current; }
};

/// @ingroup MAL
/// Shared pointer to an instance of MAL_Interface.
typedef std::shared_ptr<MAL_Interface> MAL_InterfacePtr;

} // end namespace

#endif
