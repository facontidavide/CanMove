#ifndef DOXYCHAPTER4_H
#define DOXYCHAPTER4_H

/**
 * \page chapter-4 Chapter 4: The Motor Abstract Layer (MAL)
 *
 * The Motor Abstract Layer is a generic interface to servo dirve and motor controllers that borrows its concept
 * and terminology from the CiA 402 profile (more information at http://www.can-cia.org/index.php?id=539).
 * Manufacturer of different servo drives will likecly implement only some functionalities of this standard,
 * and very often they will add custom instruction to add more functionalities (this is allowed by CANopen).
 * the MAL generally try to provide the most common functionalities through an API that is quite self-explaining,
 * as long as you do know the basic "Motion Modes" that are described by CiA 402 (explained further).
*
*
*
* \section chapter-4-1 Getting started with MAL_Interface
*
* The first thing that need to be understood is that the MAL_Interface is a layer on op of CO301_Interface.
* The relationship between the two class isn't inheritance but composition instead (i.e., the MAl get a pointer
* to a CO301_Interface * to do its stuff).
* For this reason, it is not a surprise that a MAL_Interface instance can be created extending the parameters in
* the configuration XML that we have already introduced in \ref chapter-2-3 .
*
* \code{.xml}

<CanPorts>
    <CanPort portname="can0" bitrate="1M" />
</CanPorts>

<ObjectDictionaries>
    <ObjectDictionary file="IngeniaPluto.eds"  name="pluto"/>
<ObjectDictionaries>

<Devices>
    <Device>
        <device_ID>0</device_ID>
        <can_node_ID>32</can_node_ID>
        <can_portname>can0</can_portname>
        <dictionary_name>pluto</dictionary_name>

        <Motor>
            <max_current>1000</max_current>
            <encoder_resolution>4000</encoder_resolution>
            <gear_reduction>1</gear_reduction>
            <invert_direction>false</invert_direction>
        </Motor>

    </Device>

</Devices>

* \endcode
*
* The new parameters introduced inside <Device> (grouped inside <Motor>) are:
*
* - __max_current__: it is good practice to limit the maximum current of the servo drive to prevent damaging the
*   motor/actuator. The current limitation can be changed at runtime using the command Mal_Interface::setCurrentLimit,
*   but will never be able to exceed the value specified in the parameter __max_current__. We recommend to use the values
*   provided by the manufacturer of your motor/actuators and known as "rated current".
* - __encoder_resolution__: this is the resolution of the encoder. Consider that most of the servo drives will use an interpolation
*   method that multiplies the Counts Per Rotation (CPR) by 4X. In other words, an encoder wil 1000 CPR will have a resolution of 4000.
* - __gear_reduction__: when a gearhead is added to the motor, the reduction ration can be defined here.
* - __invert_direction__: when you want to invert the positive direction of rotation, set this to "true".
*
* As it can be seen, most of these parameters are used to convert the native unit of rotation commonly used by motor
* controllers (the count ofpulses from the encoder of the motor) to the one that is seen at the output shaft of your motor
*  (or gearhead if one is used). The latter rotation is expressed either in __radians__ (default) and the velocity is __radians-per-seconds__.
*
* We can get an instance of the newly allocated CO301 interface and MAL_Interface using the functions:
*
* \code{.cpp}

// Note that device and motor are two complementary interfaces to the SAME servo drive.
CO301_InterfacePtr device = get_CO301_Interface( 0 );
MAL_InterfacePtr   motor  = get_MAL_Interface( 0 );

* \endcode
*
* Configuration of the Drive and feedback.
* -----------------------------
*
* Once the MAL_Interface has been allocated for a particular servo drive, we have to configure the drive only once.
* CAN/Moveit does a custom initialization of the PDO that will work fine for 90% of the applications.
* Simply execute the command:
*
* \code{.cpp}

// Do this at least once when the servo drive is switched on to overwrite default mapping of PDOs.
motor->configureDrive();

* \endcode
*
* In particular, this method maps PDO1_Tx and PDO2_Tx to send synchronously (when a SYNC message is broadcasted) the following information:
* - __STATUSWORD__: The register that describe the current status of the drive. The interpreted value of this status can be accessed
*  by the user using the method MAL_Interface::getStatus.
* - __POSITION_ACTUAL_VALUE__: current position of the motor. Usefull to monitor how well the given reference position is followed by
* the controller. Access the value using MAL_Interface::getActualPosition.
* - __VELOCITY_ACTUAL_VALUE__: Same as POSITION_ACTUAL_VALUE for the velocity of the drive. Access the value using MAL_Interface::getActualVelocity.
* - __CURRENT_ACTUAL_VALUE__: the motor current in milliAmps. Use MAL_Interface::getActualCurrent to read the most recend value (note that
* most of the time this value will be very noisy, especially for the PID parameters where tuned to have a stiff position control.
*
* As discussed in \ref chapter-2-5 , The local values of the ObjectDatabase will be refreshed only when the synchronous transmission of
* the PDOs_TX is triggered by the command cmi_sendSync.
*
* Alternatively, you can request fresh values from a drive using a SDO Upload (Read) request in disguise, i.e. the methods:
*
* - requestActualPosition (not needed is SYNC is used, since it is PDO-mapped).
* - requestActualCurrent (not needed is SYNC is used, since it is PDO-mapped).
* - requestActualVelocity (not needed is SYNC is used, since it is PDO-mapped).
* - requestPositionError (position error is not mapped by default on any PDO_TX).
*
* Start, stop and fault.
* -----------------------------
*
* CANopen profile CiA 402 define a standard state machine with the following states:
*
* \image html states_402.jpg
*
* The current state can be accessed using the command MAL_Interface::getStatus.
*
* If you want to switch the drive to the state OPERATION_ENABLED (where the motor is actually controlled), you
* can use the command MAL_Interface::startDrive.
* When you want the position/velocity/torque controller to be switched off, invoke instead MAL_Interface::stopDrive.
*
* Currently, the method MAL_Interface::resetFault is equivalent to startDrive. We provided two different methods to allow
* further customization of these actions in future version of CAN/MoveIt.
*





* \section chapter-4-2 Profiled Velocity Mode
*
* This mode is used to control the rotational speed (velocity) of the drive; the reference speed is profided vy the use.
* The transition from one velocity to another will be done using a phase of constant acceleration/deceleration that shall be
* modifed before setting the reference velocity.
*
* \image html profiled_velocity.png
*
* To start spinning a motor simply execute:
*
* \code{.cpp}

 // We suppose that configureDrive and startDrive have been called already
 motor->setModeOperation( PROFILED_VELOCITY_MODE );

 // acceleration is expressed in rad/(sec^2)
 motor->setProfileParameter( PROFILE_ACCELERATION, 100);
 motor->setProfileParameter( PROFILE_DECELERATION, 100);

 // spin at 50 rad/sec
 motor->setProfiledVelocityTarget( 50 );

* \endcode
*




* \section  chapter-4-3 Profiled Position Mode
*
* In PROFILED_POSITION_MODE, the motor moves to the target (reference) position specified by the user using what is knows
* as a trapezoidal interpolation.
* In other words, the target position is reached using an acceleration phase, a phase at constant velocity and finally a
* deceleration phase (final velocity must be zero by default).
*
* \image html trapezoid.png
*
* The image above is just a qualitative example to visualize these phases.
* To quickly test this mode you can try the following commands.
*
* \code{.cpp}
*
 motor[i]->setModeOperation( PROFILED_POSITION_MODE);
 motor[i]->setProfileParameter( PROFILE_ACCELERATION, 500);
 motor[i]->setProfileParameter( PROFILE_DECELERATION, 500);
 motor[i]->setProfileParameter( PROFILE_VELOCITY,50);

 // start moving
 motor[i]->setProfiledPositionTarget( 100 );

 //This time should be sufficient.
 sleep_for( Seconds(3) );

 // Come back to the initial position.
 motor[i]->setProfiledPositionTarget( 0 );

* \endcode
*
* It should be noted that bu default the target position is absolute. You can also specify a relative position
* using the following additional flag.
*
* \code{.cpp}

 // Move relatively to the current position.
 motor[i]->setProfiledPositionTarget( 100 , PROFILE_RELATIVE_POS);

* \endcode
*
* Another interesting option that is also available on most of the servo drive is to concatenate more than one target point
* using a buffered queue. When this option is used, only the final target will be reached with zero velocity but not the intermediate ones.
*
* \code{.cpp}

 // Supposing that the actual position is less than 100, the first target position will
 // be reached with non-zero velocity.
 motor[i]->setProfiledPositionTarget( 100 , PROFILE_BUFFERED_POINT);

 // Inversion of direction: velocity at target will be zero.
 motor[i]->setProfiledPositionTarget( 200 , PROFILE_BUFFERED_POINT);

 // Last point of the queue: velocity at target will be zero.
 motor[i]->setProfiledPositionTarget( 0 , PROFILE_BUFFERED_POINT);

* \endcode
*
* There is also the option (as described in \ref chapter-3) to have an event associated to this mode.
* The event will be emitted when the specified target is reached; if the buffered mode is used, an event is created for each
* of the targets.
*
* \code{.cpp}

 motor->events()->add_subscription(EVENT_PROFILED_POSITION_REACHED,  ... , ... );

* \endcode
*


* \section  chapter-4-4 Interpolate Position Mode
*
* When this mode is used the master application must send a streaming of positions.
* The controller will try to pass through these targets, using a fixed time delay delta_t between them.
* The kind of interpolation between these point is specified by the manufacturer, but by default the simplest one is used,
* i.e. linear interpolation; for this reason the user should consider a small delta_t between points if a smooth interpolation
* in needed.
*
* \image html interpolated_pos.png
*
* The size of the buffer is limited and can differ between different models/brands of servo drives.
* To know the maximum size and avoid overflow you might want access the object entries:
*
* - __Maximum size of buffer__: ObjectID(0x60C4 , 1).
* - __Actual buffer size__: ObjectID(0x60C4 , 2).
*
* A very simple example of the INTERPOLATED_POSITION_MODE can be:
*
*  \code {.cpp}

  Milliseconds dT(100);
  motor->setModeOperation( INTERPOLATED_POSITION_MODE);

  // Note: it is mandatory to specify this parameter
  motor->setInterpolatedPositionPeriod( PERIOD );

  double t = 0;

  while(t < 3.14)
  {
     t += 0.001* dT.count();
     double reference_pos =  10*sin(t);
     motor->pushInterpolatedPositionTarget( reference_pos );
  }

* \endcode





* \section chapter-4-5 Torque Mode
*
* The current version of TORQUE_MODE is a simplification of the "Profiled Torque Mode".
* It simply express the torque of the motor through its current. It is up to the user to apply the Nm/Amp conversion when
* this is provided by the datasheet of the motor.
*
* \warning We will lickely extend/modify this interface in future version of CMI. If you need to have a more fine grained control of
* this mode, we recommend to access directly the related objects using the CO301_Interface.
*
* \code{.cpp}

 // We suppose that configureDrive and startDrive have been called already.
 motor->setModeOperation( TORQUE_MODE );

 // Torque generated by 100 milliAmps
 motor->setCurrentTarget( 100 );

* \endcode
*
* */

#endif // DOXYCHAPTER4_H
