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

#include "spdlog/fmt/fmt.h"
#include "cmi/MAL_types.h"
#include "cmi/MAL_CANOpen402.h"
#include "cmi/CO402_def.h"


namespace CanMoveIt{

class MAL_CANOpen402::Impl
{
public:
    MAL_CANOpen402     *self;
    CO301_InterfacePtr CO_interface;
    bool               first_interpolated_pos;
    bool               autostart_interpolated_pos;
    bool               drive_configured;
    bool               IP_is_PVT;

    ObjectKey          controlword;
    ObjectKey          statusword ;

    ObjectKey          profiled_acceleration;
    ObjectKey          profiled_deceleration;
    ObjectKey          profiled_velocity;
    ObjectKey          current_actual_value;

    uint16_t           raw_statusword;
    uint8_t            IP_period;
    MotorStatus        status;
    uint8_t              pending_position_reached;

    void			   modeDisplayUpdate(uint16_t, EventData const&);

    Impl(MAL_CANOpen402* s, CO301_InterfacePtr co):
        self( s ),
        CO_interface( co ),
        first_interpolated_pos(false),
        autostart_interpolated_pos(true),
        drive_configured(false),
        IP_is_PVT(false),
        controlword(0xFF),
        statusword(0xFF),
        profiled_acceleration(0xFF),
        profiled_deceleration(0xFF),
        profiled_velocity(0xFF),
        current_actual_value(0xFF),
        raw_statusword(0),
        IP_period(1),
        status( STATUS_NOT_INITIALIZED ),
        pending_position_reached(0)
    {

    }
};


MAL_CANOpen402::MAL_CANOpen402(int ID, CO301_InterfacePtr co301_ptr, uint32_t max_safety_current):
    MAL_Interface(ID, max_safety_current),
    _d(new Impl(this,co301_ptr ))
{
    using namespace std::placeholders;

    InterpreterCallback callback =  std::bind(&MAL_CANOpen402::msgReceivedInterpreter, this, _1);
    co301()->addReadInterpreter( callback );

    EventCallback status_update = std::bind(&MAL_CANOpen402::parseStatusWord, this, _1, _2);
    co301()->events()->add_subscription(STATUSWORD.get(), CALLBACK_SYNCH_CANREAD, status_update);

    EventCallback mode_display_update = std::bind(&MAL_CANOpen402::Impl::modeDisplayUpdate, _d, _1, _2);
    co301()->events()->add_subscription(MODE_OPERATION_DISPLAY.get(), CALLBACK_SYNCH, mode_display_update);

    _d->controlword           = co301()->findObjectKey( CONTROLWORD );
    _d->statusword            = co301()->findObjectKey( STATUSWORD );

    _d->profiled_acceleration = co301()->findObjectKey( PROFILED_ACCELERATION );
    _d->profiled_deceleration = co301()->findObjectKey( PROFILED_DECELERATION );
    _d->profiled_velocity     = co301()->findObjectKey( PROFILED_VELOCITY );

    try{
        _d->current_actual_value = co301()->findObjectKey( CURRENT_ACTUAL_VALUE );
    }
    catch(std::runtime_error)
    {
        try { _d->current_actual_value = co301()->findObjectKey( TORQUE_DEMAND_VALUE ); }
        catch(std::runtime_error) {};
    }
}

MAL_CANOpen402::~MAL_CANOpen402()
{
    delete _d;
}

CO301_InterfacePtr MAL_CANOpen402::co301()
{
    return _d->CO_interface;
}

int16_t MAL_CANOpen402::getID() const
{
    return _d->CO_interface->device_ID();
}

EventDispatcher* MAL_CANOpen402::events()
{
    return co301()->events() ;
}

bool MAL_CANOpen402::isMotorReady()
{
    if (this->_status == OPERATION_ENABLED) return true;
    else return false;
}

void MAL_CANOpen402::setInterpolatedPositionAutostart(bool autostart)
{
    _d->autostart_interpolated_pos = autostart;
}

void MAL_CANOpen402::sendInterpolatedPositionStart()
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = 0x00;
    co301()->push_PDO_RX(PDO3_RX, 2, data);

    data[0] = 0x1F;
    co301()->push_PDO_RX(PDO3_RX, 2, data);
}

CommandResult MAL_CANOpen402::configureDrive(const char*)
{
    Log::MAL()->info("start configuring motor {}", getID() ) ;
    co301()->sdoObjectRequest( co301()->findObjectKey(POSITION_ACTUAL_VALUE) );

    //-----------------------------------
    //start PDO mapping
    co301()->sendNMT_stateChange(NMT_PRE_OPERATIONAL);

    //--------- PDO1_RX_Map for target position
    try{
        co301()->findObjectKey( ObjectID( INTERPOLATED_DATA_RECORD, 1) );
        PDO_MappingList obj_list;
        obj_list.push_back ( ObjectID( INTERPOLATED_DATA_RECORD, 1) );

        co301()->pdoMapping(PDO1_RX, obj_list);
    }
    catch(std::runtime_error)
    {
        // TODO temporary hack for EPOS2
        if(co301()->getObjectDictionary()->vendorNumber()   == 0x000000FB &&
                co301()->getObjectDictionary()->productNumber()  == 0x63220000  )
        {
            PDO_MappingList obj_list;
            obj_list.push_back ( ObjectID( 0x20C1, 0) );
            co301()->pdoMapping(PDO1_RX, obj_list);
        }
    }
    //-------- PDO2_RX_Map for target torque
    try
    {
        co301()->findObjectKey( TARGET_TORQUE );
        PDO_MappingList obj_list;
        obj_list.push_back ( TARGET_TORQUE );
        co301()->pdoMapping(PDO2_RX, obj_list);
    }
    catch(std::runtime_error) {}

    //-------- PDO3_RX_Map for controlword
    {
        PDO_MappingList obj_list;
        obj_list.push_back ( CONTROLWORD );
        co301()->pdoMapping(PDO3_RX, obj_list); // TODO: this is not broadcasted
    }
    //---------- PDO1_TX_Map for actual position, position error and actual torque
    {
        PDO_MappingList obj_list;
        obj_list.push_back ( POSITION_ACTUAL_VALUE );
        obj_list.push_back ( VELOCITY_ACTUAL_VALUE );
        co301()->pdoMapping(PDO1_TX, obj_list);

        co301()->pdoSetTransmissionType_Synch(PDO1_TX, 1 );
    }
    //---------- PDO2_TX_Map
    {
        PDO_MappingList obj_list;
        obj_list.push_back ( STATUSWORD );

        if( _d->current_actual_value != ObjectKey(0xFF))
        {
            obj_list.push_back ( _d->CO_interface->getObjectID( _d->current_actual_value )  );
        }
        co301()->pdoMapping(PDO2_TX, obj_list);
        co301()->pdoSetTransmissionType_Synch(PDO2_TX, 1 );
    }

    //---------- PDO3_TX_Map is asynchronous
    {
        PDO_MappingList obj_list;
        obj_list.push_back ( STATUSWORD );
        co301()->pdoMapping(PDO3_TX, obj_list);
        co301()->pdoSetTransmissionType_ASynch(PDO3_TX, ASYNCH_PROFILE, Milliseconds(1), Milliseconds(100) );
    }
    //----------

    // disable by default. Enable only in profiled-position
    co301()->pdoEnableComm( PDO3_RX, false );

    // not defined here, therefore disabled.
    co301()->pdoEnableComm( PDO4_TX, false );

    co301()->sendNMT_stateChange(NMT_OPERATIONAL);
    Log::MAL()->info("start configuring motor{} DONE", getID() ) ;
    // -----------------------------------

    co301()->sdoObjectRequest( _d->profiled_acceleration );
    co301()->sdoObjectRequest( _d->profiled_deceleration );
    co301()->sdoObjectRequest( _d->profiled_velocity );

    try{
        co301()->sdoObjectRequest( co301()->findObjectKey(RATE_CURRENT) );
    }
    catch (std::runtime_error){}

    _d->drive_configured = true;
    // TODO. Always SUCCESS?
    return SUCCESSFUL;
}

void MAL_CANOpen402::resetFaults()
{
    startDrive();
}

FaultType MAL_CANOpen402::getLastFault()
{
    Log::MAL()->warn("getLastFault not implemented yet");
    return NO_FAULT;
}

CommandResult MAL_CANOpen402::startDrive()
{
    if( !_d->drive_configured)
    {
        this->configureDrive();
    }

    co301()->waitQueueEmpty(Milliseconds(1000));
    Log::MAL()->info("-------------------------\nstartDrive begin");

    co301()->sendNMT_stateChange(NMT_OPERATIONAL);
    _status = STATUS_NOT_INITIALIZED;

    int attempts = 0;
    do {
        Variant sw_value = 0xFF;
        co301()->sdoRequestAndGet( _d->statusword, &sw_value, Microseconds(100*1000) );

        if( sw_value != 0xFF )
        {
            Log::MAL()->debug("status word (0x{:X}) {}: {} ",
                           sw_value.convert<uint16_t>(),
                           _status, StatusToString( _status ) );

            switch( this->_status)
            {
            case SWITCH_ON_DISABLED: co301()->sdoWrite(_d->controlword, 0x06); break;
            case READY_TO_SWITCH_ON:
            {
                co301()->sdoWrite(_d->controlword, 0x07);
                co301()->sdoWrite(_d->controlword, 0x0F);
                sleepFor(Milliseconds(100));
            }break;
            case SWITCHED_ON:        co301()->sdoWrite(_d->controlword, 0x0F); break;
            case FAULT_STATE:{
                co301()->sdoWrite(_d->controlword, 0x00);
                co301()->sdoWrite(_d->controlword, 0x80);
                co301()->sdoWrite(_d->controlword, 0x06);
                Log::MAL()->info("fault recovery" );
            }break;
            case QUICK_STOP_ACTIVE: co301()->sdoWrite(_d->controlword, 0x0F); break;
            case OPERATION_ENABLED: break;

            default:{
                Log::MAL()->warn("startDrive doesn't know what to do on status {} ", _status);
            } break;
            }
            co301()->waitQueueEmpty( Milliseconds(100) );
        }
        if( attempts++ > 50)
        {
            return ERROR_NOT_READY;
        }

    }while (_status != OPERATION_ENABLED );

    co301()->sdoObjectRequest( co301()->findObjectKey( POSITION_ACTUAL_VALUE) );

    co301()->waitQueueEmpty( Milliseconds (10));
    Log::MAL()->info("startDrive finished.");
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::setPID(double P, double I, double D)
{
    Log::MAL()->error(" MAL_CANOpen402::setPID not implemented yet");
    return ERROR_NOT_AVAILABLE;
}

CommandResult MAL_CANOpen402::setModeOperation(ModeOperation mode, bool force)
{
    if( _mode_operation == mode && force == false)
    {
        return SUCCESSFUL;
    }

    this->co301()->sdoWrite( co301()->findObjectKey( MODE_OPERATION), (int8_t) mode);
    this->co301()->sdoObjectRequest( co301()->findObjectKey( MODE_OPERATION_DISPLAY) );

    Log::MAL()->info("setModeOperation : {}", mode );

    if (mode == INTERPOLATED_POSITION_MODE)
    {
        // this will stop the drive and clear the buffer

        co301()->sdoWrite( _d->controlword,  (uint16_t) 0x0F);
        co301()->sdoWrite( co301()->findObjectKey(0x60C4,6), 0);
        co301()->sdoWrite( co301()->findObjectKey(0x60C4,6), 1);
        _d->first_interpolated_pos = true;
        co301()->pdoEnableComm( PDO3_RX, true );
    }
    else{
        co301()->pdoEnableComm( PDO3_RX, false );
    }

    if( mode == PROFILED_POSITION_MODE)
    {
        co301()->sdoWrite(_d->controlword, 0x2F);
        co301()->sdoWrite(_d->controlword, 0x3F);
    }

    _mode_operation = mode;
    return SUCCESSFUL;
}

void MAL_CANOpen402::stopDrive()
{
    //  co301()->sendNMT_stateChange(NMT_PRE_OPERATIONAL);
    // CHANGED to switchoff
    co301()->sdoWrite( _d->controlword, (uint16_t) 0x06 );
}


CommandResult MAL_CANOpen402::pushInterpolatedPositionTarget(double pos_in_rad, double vel_rad_sec)
{
    ObjectKey interpolated_data;
    try{
        interpolated_data =  co301()->findObjectKey( INTERPOLATED_DATA_RECORD, 1);
    }
    catch(std::runtime_error) { interpolated_data = ObjectKey(0xFF) ; }

    if (this->_mode_operation != INTERPOLATED_POSITION_MODE)
    {
        return ERROR_WRONG_MODE;
    }
    if( this->_status != OPERATION_ENABLED)
    {
        return ERROR_NOT_READY;
    }

    int32_t int_pos_ref = (pos_in_rad  * _rad_to_encoder);
    int32_t int_vel_ref = (vel_rad_sec * _rad_to_encoder);
    uint8_t data[8];
    uint8_t data_length = 0;

    if( interpolated_data != ObjectKey(0xFF) )
    {
        data[0] = (int_pos_ref)    & 0xFF;
        data[1] = (int_pos_ref>>8) & 0xFF;
        data[2] = (int_pos_ref>>16) & 0xFF;
        data[3] = (int_pos_ref>>24) & 0xFF;
        data_length = 4;
    }
    else if( co301()->getObjectDictionary()->vendorNumber()   == 0x000000FB &&
             co301()->getObjectDictionary()->productNumber()  == 0x63220000  )
    {
        data[0] = (_d->IP_period);

        data[1] = (int_vel_ref)     & 0xFF;
        data[2] = (int_vel_ref>>8)  & 0xFF;
        data[3] = (int_vel_ref>>16) & 0xFF;

        data[4] = (int_pos_ref)     & 0xFF;
        data[5] = (int_pos_ref>>8)  & 0xFF;
        data[6] = (int_pos_ref>>16) & 0xFF;
        data[7] = (int_pos_ref>>24) & 0xFF;

        data_length = 8;
    }
    co301()->push_PDO_RX(PDO1_RX, data_length, data);

    if( _d->first_interpolated_pos && _d->autostart_interpolated_pos )
    {
        Log::MAL()->info(" first_interpolated_pos A" );

        co301()->sdoWrite( _d->controlword,  (uint16_t) 0x0F);
        co301()->sdoWrite( co301()->findObjectKey(0x60C4,6), 1);
        co301()->sdoWrite( co301()->findObjectKey(0x60C4,6), 0);

        co301()->push_PDO_RX(PDO1_RX, data_length, data);

        co301()->sdoWrite(_d->controlword,  (uint16_t) 0x1F);
        Log::MAL()->info(" first_interpolated_pos B");
        _d->first_interpolated_pos = false;
    }
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::setProfiledPositionTarget(double pos_in_rad, uint16_t flags)
{
    static ObjectKey target_position = co301()->findObjectKey( TARGET_POSITION );

    if (this->_mode_operation != PROFILED_POSITION_MODE)
    {
        return ERROR_WRONG_MODE;
    }
    if( this->_status != OPERATION_ENABLED)
    {
        return ERROR_NOT_READY;
    }

    int32_t int_pos_ref = static_cast<int32_t>(pos_in_rad*_rad_to_encoder);

    if ( ! (flags & PROFILE_BUFFERED_POINT) )
    {
        flags |= PROFILE_CHANGE_IMMEDIATELY;
    }

    _d->pending_position_reached++;
    co301()->sdoWrite(target_position, int_pos_ref);
    co301()->sdoWrite(_d->controlword, 0x0F | flags);
    co301()->sdoWrite(_d->controlword, 0x1F | flags);

    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::setProfiledVelocityTarget(double rad_sec)
{
    if (this->_mode_operation != PROFILED_VELOCITY_MODE)
    {
        return ERROR_WRONG_MODE;
    }
    if( this->_status != OPERATION_ENABLED)
    {
        return ERROR_NOT_READY;
    }

    int32_t int_vel_ref = (rad_sec*_rad_to_encoder);

    ObjectKey target_velocity = co301()->findObjectKey( TARGET_VELOCITY );

    co301()->sdoWrite(_d->controlword, 0x0F);
    co301()->sdoWrite( target_velocity, int_vel_ref);
    co301()->sdoWrite(_d->controlword, 0x0F);

    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::setInterpolatedPositionPeriod(Milliseconds milliseconds)
{
    if (_mode_operation == INTERPOLATED_POSITION_MODE)
    {
        int64_t msec =  milliseconds.count();
        if( msec > 254)
        {
            Log::MAL()->error("CANOpen supports a maximum InterpolatedPositionPeriod of 254 milliseconds only") ;
            return ERROR_WRONG_PARAMETER;
        }
        this->co301()->sdoWrite( co301()->findObjectKey(0x60C2, 1) , msec );
        _d->IP_period = msec;
        return SUCCESSFUL;
    }
    else{
        return ERROR_WRONG_MODE;
    }
}

CommandResult MAL_CANOpen402::setCurrentTarget(int32_t curr_in_mA)
{
    if( _rad_to_encoder < 0 ) curr_in_mA = -curr_in_mA;

    if( curr_in_mA >  _safety_max_current)
        curr_in_mA =  _safety_max_current;

    if( curr_in_mA < -_safety_max_current)
        curr_in_mA = -_safety_max_current;

    uint8_t msg_data[2];
    msg_data[0] = (curr_in_mA)    & 0xFF;
    msg_data[1] = (curr_in_mA>>8) & 0xFF;

    co301()->push_PDO_RX( PDO2_RX, 2, msg_data);

    return SUCCESSFUL;
}


CommandResult MAL_CANOpen402::setCurrentLimit(uint32_t curr_in_mA)
{
    if ( curr_in_mA > (unsigned)_safety_max_current)
    {
        Log::MAL()->warn("current limit clamped to value {}", _safety_max_current);
        curr_in_mA = _safety_max_current;
    }

    try{
        static ObjectKey rated_torque = co301()->findObjectKey( RATE_TORQUE );
        co301()->sdoWrite( rated_torque,(uint32_t)curr_in_mA );
        return SUCCESSFUL;
    }
    catch(std::runtime_error e)
    {
        Log::MAL()->error("Object 0x6076 not found in dictionary. setCurrentLimit not available");
        return ERROR_NOT_AVAILABLE;
    }
}

CommandResult MAL_CANOpen402::setProfileParameter(ProfileParameter param, double value)
{
    if (value <0  )
        return ERROR_VALUE_OUT_OF_RANGE;

    uint32_t value_i = value * _rad_to_encoder;

    if (param == PROFILE_ACCELERATION )
    {
        co301()->sdoWrite( _d->profiled_acceleration, value_i);
    }
    else  if (param == PROFILE_DECELERATION )
    {
        co301()->sdoWrite( _d->profiled_deceleration, value_i);
    }
    else if (param == PROFILE_VELOCITY  )
    {
        co301()->sdoWrite( _d->profiled_velocity , value_i);
    }

    co301()->sdoWrite(_d->controlword, 0x0F);
    co301()->sdoWrite(_d->controlword, 0x1F);
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::haltProfiledPosition()
{
    if (this->_mode_operation != PROFILED_POSITION_MODE)
        return ERROR_WRONG_MODE;

    uint16_t control_word = 0x0F;
    co301()->sdoWrite( _d->controlword, control_word);

    control_word = 0x1F | PROFILE_HALT;
    co301()->sdoWrite( _d->controlword, control_word);

    return SUCCESSFUL;
}

void MAL_CANOpen402::getProfileParameter(ProfileParameter param, double *value)
{
    Variant value_a;

    if (param == PROFILE_ACCELERATION)
        co301()->getLastObjectReceived( _d->profiled_acceleration, &value_a);

    else if (param == PROFILE_DECELERATION)
        co301()->getLastObjectReceived( _d->profiled_deceleration, &value_a);

    else if (param == PROFILE_VELOCITY  )
        co301()->getLastObjectReceived( _d->profiled_velocity, &value_a);

    *value = ( value_a.convert<float>() ) / _rad_to_encoder ;
}

bool MAL_CANOpen402::msgReceivedInterpreter(const CanMessage & m)
{
    bool recognized = false;
    return recognized;
}

DataStatus MAL_CANOpen402::getPositionError(double* pos_err)
{
    Variant temp_pos( 0 );
    DataStatus output = co301()->getLastObjectReceived( co301()->findObjectKey( FOLLOWING_ERROR_ACTUAL ), &temp_pos);
    *pos_err = ( temp_pos.convert<double>() )/_rad_to_encoder;
    if( output == DS_NO_DATA) *pos_err = 0;
    return output;
}
DataStatus MAL_CANOpen402::getActualPosition(double* act_pos)
{
    Variant temp_pos( 0 );

    DataStatus output = co301()->getLastObjectReceived( co301()->findObjectKey( POSITION_ACTUAL_VALUE ), &temp_pos);
    *act_pos = ( temp_pos.convert<double>() )/_rad_to_encoder;
    if( output == DS_NO_DATA) *act_pos = 0;
    return output;

}
DataStatus MAL_CANOpen402::getActualCurrent(int32_t* act_curr)
{ 
    DataStatus res ;
    Variant current_actual( 0 );

    res = co301()->getLastObjectReceived( _d->current_actual_value, &current_actual);

    *act_curr = ( current_actual.convert<int32_t>() );

    try{
        ObjectKey rated_current_key = co301()->findObjectKey( RATE_CURRENT );
        Variant rated_current_value;
        co301()->getLastObjectReceived( rated_current_key, &rated_current_value);
        *act_curr = (rated_current_value.convert<int32_t>() * (*act_curr) )/1000;
    }
    catch (std::runtime_error) {}
    return res;
}

DataStatus    MAL_CANOpen402::getActualVelocity(double* act_vel)
{
    Variant temp_vel( 0 );
    DataStatus output =  co301()->getLastObjectReceived( co301()->findObjectKey( VELOCITY_ACTUAL_VALUE ), &temp_vel);

    *act_vel = (temp_vel.convert<double>() )/_rad_to_encoder;
    if( output == DS_NO_DATA) *act_vel = 0;
    return output;
}

void MAL_CANOpen402::getCurrentLimit(uint32_t *current_limit)
{
    try{
        static ObjectKey rated_torque = co301()->findObjectKey( RATE_TORQUE );

        Variant curr_lim;
        co301()->getLastObjectReceived( rated_torque, &curr_lim);
        *current_limit = curr_lim.convert<uint32_t>();
    }
    catch(std::runtime_error e)
    {
        Log::MAL()->error( e.what() );
    }
}
void  MAL_CANOpen402::getProfiledPositionAcceleration(double* max_acc, double *max_dec)
{
    Variant temp_acc (0);
    co301()->getLastObjectReceived( _d->profiled_acceleration, &temp_acc);
    *max_acc = ( temp_acc.convert<double>() ) / _rad_to_encoder;

    co301()->getLastObjectReceived( _d->profiled_deceleration, &temp_acc);
    *max_dec = ( temp_acc.convert<double>() ) / _rad_to_encoder;
}

void  MAL_CANOpen402::getProfiledPositionVelocity(double* max_vel)
{
    Variant temp_vel (0);
    co301()->getLastObjectReceived(_d->profiled_velocity, &temp_vel);
    *max_vel = ( temp_vel.convert<double>() ) / _rad_to_encoder;
}

CommandResult MAL_CANOpen402::requestActualPosition()
{
    co301()->sdoObjectRequest( co301()->findObjectKey( POSITION_ACTUAL_VALUE ) );
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::requestActualCurrent()
{
    co301()->sdoObjectRequest( co301()->findObjectKey( TORQUE_DEMAND_VALUE) );
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::requestActualVelocity()
{
    co301()->sdoObjectRequest( co301()->findObjectKey( VELOCITY_ACTUAL_VALUE) );
    return SUCCESSFUL;
}

CommandResult MAL_CANOpen402::requestPositionError()
{
    co301()->sdoObjectRequest( co301()->findObjectKey( FOLLOWING_ERROR_ACTUAL) );
    return SUCCESSFUL;
}

MotorStatus MAL_CANOpen402::getStatus()
{
    return _status;
}


void MAL_CANOpen402::parseStatusWord(uint16_t, EventData const& event)
{
    EventDataObjectUpdated object = absl::any_cast<EventDataObjectUpdated>( event.info);

    static uint16_t _prev_statusword = 0xFFFF;
    uint16_t sw_value = object.data.extract<uint16_t>();

    uint16_t temp = sw_value & 0x4F; // x1xx 1111

    if(temp == 0x00 )       _status = NOT_READY;
    else if(temp == 0x40 )  _status = SWITCH_ON_DISABLED;
    else if(temp == 0x0F )  _status = FAULT_REACTION;
    else if(temp == 0x08 )  _status = FAULT_STATE;
    else {
        temp = sw_value & 0x6F; // x11x 1111

        if(temp == 0x21 )      _status = READY_TO_SWITCH_ON;
        else if(temp == 0x23 ) _status = SWITCHED_ON;
        else if(temp == 0x27 ) _status = OPERATION_ENABLED;
        else if(temp == 0x07 ) _status = QUICK_STOP_ACTIVE;
        else {
            Log::MAL()->error("StatusWord not recognized !!! 0x{:X}", sw_value);
            _status = STATUS_NOT_INITIALIZED;
        }
    }

    Log::MAL()->debug(" status 0x{:X} ( 0x{:X}  / {})", sw_value, _prev_statusword, _mode_operation);

    if( this->_mode_operation == PROFILED_POSITION_MODE &&
            ( _prev_statusword & 0x400) == 0 &&
            ( sw_value         & 0x400) != 0 &&
            _d->pending_position_reached > 0 )
    {
        EventData event;
        event.event_id = EVENT_PROFILED_POSITION_REACHED;
        event.timestamp = GetTimeNow();
        this->co301()->events()->push_event(co301()->device_ID() , event );

        //TODO: this is not thread safe yet
        _d->pending_position_reached--;

        Log::MAL()->debug("pushing EVENT_PROFILED_POSITION_REACHED");
    }

    _prev_statusword = sw_value;
}

void MAL_CANOpen402::Impl::modeDisplayUpdate(uint16_t, EventData const& event)
{
    EventDataObjectUpdated object = absl::any_cast<EventDataObjectUpdated>( event.info);
    self->_mode_operation = static_cast<ModeOperation> ( object.data.convert<int8_t>() );
}

/*
CommandResult MAL_CANOpen402::configureDrive(const char *filename)
{
    ///   FIXME
     *
     *using boost::property_tree::ptree;
    ptree pt;
    read_xml(filename, pt);

    for (int i=0; i<4; i++)
    {
        std::string node_name("PDO_Mapping.PDO_TX_");
        std::ostringstream s_number;
        s_number << i;
        node_name.append( s_number.str());

        boost::property_tree::ptree::value_type PDO_node = pt.get_child_optional(node_name);
        //------------------------------------------------------
        int new_cob_id = DEFAULT_PDO1_RX + (i* 0x100) + co301()->getNode();

        std::string cobid = PDO_node.second.get<std::string>("cobid", "use_default");
        if (cobid.compare("use_default") != 0)
        {
            new_cob_id = hexStringToInt( cobid );
        }

        co301()->pdoStartMapping(PDO1_TX_Map + i, new_cob_id);
        //------------------------------------------------------
        BOOST_FOREACH(ptree::value_type &v, PDO_node.second.get_child("ObjectsToMap"))
        {
            ObjectID key( v.second.get<uint16_t>("<xmlattr>.index"),
                             v.second.get<uint16_t>("<xmlattr>.subindex") );
           co301()->pdoMapObject(key);
        }
        //------------------------------------------------------
        std::string comm_mode  = PDO_node.second.get<std::string>("Comm.<xmlattr>.type");
        int         comm_param = PDO_node.second.get< int >("Comm.<xmlattr>.param");

        if( comm_mode.compare("asynch") == 0)
        {
            co301()->pdoSetTransmissionType_ASynch(comm_param);
        }
        else if ( comm_mode.compare("synch") == 0)
        {
            co301()->pdoSetTransmissionType_Synch(comm_param);
        }
        else std::runtime_error("Error in XML passed to configureDrive: PDO_TX.Comm.param");
        //------------------------------------------------------
        co301()->pdoFinishMapping();
    }

    return SUCCESSFUL;
}
*/

} // end namespace

