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

#ifndef _MalCANOPEN402__MOTOR_H
#define _MalCANOPEN402__MOTOR_H

#include "cmi/log.h"
#include "cmi/MAL_Interface.h"
#include "cmi/CO301_interface.h"



namespace CanMoveIt{


class MAL_CANOpen402: public MAL_Interface
{

public:
    MotorStatus _status;
    MAL_CANOpen402(int ID, CO301_InterfacePtr co301_ptr, uint32_t max_safety_current = 10000);

    virtual ~MAL_CANOpen402();

    int16_t getID() const ;

    virtual CommandResult	setPID(double P, double I, double D);
    virtual CommandResult	configureDrive(const char *filename = NULL);
    virtual CommandResult	startDrive();
    virtual void			stopDrive();
    virtual CommandResult	setModeOperation(ModeOperation mode, bool force=false) ;
    virtual CommandResult	pushInterpolatedPositionTarget(double pos_in_rad, double vel_rad_sec=0) ;

    virtual CommandResult	setCurrentTarget(int32_t curr_in_mA) ;
    virtual CommandResult	setCurrentLimit(uint32_t curr_in_mA) ;

    virtual CommandResult   setProfileParameter(ProfileParameter param, double value);
    virtual void            getProfileParameter(ProfileParameter param, double *value);

    virtual CommandResult	setProfiledPositionTarget(double pos_in_rad, uint16_t flags);
    virtual CommandResult   setProfiledVelocityTarget(double rad_sec);

    virtual CommandResult	haltProfiledPosition();

    virtual CommandResult   setInterpolatedPositionPeriod(Milliseconds milliseconds);

    virtual void setInterpolatedPositionAutostart(bool autostart);
    virtual void sendInterpolatedPositionStart();

    virtual void			getCurrentLimit(uint32_t *current_limit);
    virtual void			getProfiledPositionAcceleration(double* max_acc, double *max_dec);
    virtual void			getProfiledPositionVelocity(double* max_vel);

    virtual MotorStatus		getStatus();

    virtual DataStatus		getPositionError(double*);
    virtual DataStatus		getActualPosition(double*);
    virtual DataStatus		getActualCurrent(int32_t*);
    virtual DataStatus      getActualVelocity(double *vel);

    virtual CommandResult	requestActualPosition();
    virtual CommandResult	requestActualCurrent();
    virtual CommandResult	requestPositionError();
    virtual CommandResult   requestActualVelocity();

    void			   parseStatusWord(uint16_t,   EventData const&);


    virtual void       resetFaults();
    FaultType          getLastFault();

    bool	           isCanOpen() {return true;}
    EventDispatcher*   events();
    bool               isMotorReady();
    CO301_InterfacePtr co301();

private:
    class Impl;
    Impl* _d;

    virtual bool msgReceivedInterpreter(const CanMessage & m);

};


inline CO301_InterfacePtr cmi_getCO301_Interface(MAL_InterfacePtr motor_ptr )
{
    if(motor_ptr->isCanOpen() == false )
    {
        Log::CO301()->error("this is not a CANopen motor interface" );
        return CO301_InterfacePtr();
    }
    MAL_CANOpen402 *temp = static_cast<MAL_CANOpen402 *> (motor_ptr.get());
    return temp->co301();
}

}


#endif // Venus_MOTOR_H
