/**
* \page tutorial-3 Interpolated position
*
* In this example we will familiarize with the methods of CanMoveIt::MAL_Interface related to the
* INTERPOLATED_POSITION_MODE.
* We will also use the CanMoveIt::PeriodicTask utility to send commands and read states of the drive periodically.
*
* First of all, we create a task and we define a simple main as we did in the previous example:
*
* \snippet tutorials/Tutorial-IP.cpp T3-A
*
* In the __startHook__ method we initialize the pointer "motor" assuming that the XML config file has
* defined a device with ID equal to 0.
*
* Later we call the function cmi_configureMotor that will basically:
*
* - Map the PDOs to access efficiently commonly used Object Enties.
* - Change the state of the device to "OPERATIONAL".
* - Read some preliminary information from the device.
*
* Note that this Mode of Operation requires two additional parameters to be defined by the user:
*
* - The period between consecutive positions, set by setInterpolatedPositionPeriod.
* - A flag that tells to the application if the motion shall be started automatically (when the first position is pushed)
*  or explicitly by the user.
*
* Note that the latter approach (USE_AUTOSTART=false) is needed when you want to have a very tightly coordinated motion
* between multiple axis (if you accept a time offset of 1-5 milliseconds between axis, you might prefer to set USE_AUTOSTART = true).
*
* \snippet tutorials/Tutorial-IP.cpp T3-B
*
* In the main loop we:
*
* - Invoke cmi_sendSync. This is mandatory because it will trigger the transmission of some PDO_Tx previously mapped.
* - Read the Actual Position (one of the Object Entries mapped inside a PDO_Tx).
* - Push a new target position.
*
* \snippet tutorials/Tutorial-IP.cpp T3-C
*
*
* */


/**
* \page tutorial-4a Profiled position A
*
* In this tutorial we move multiple motors. Since the configuration parameters and the target positions are
* exactly the same, we expect them to move in an almost identical way.
* We subscribe to EVENT_PROFILED_POSITION_REACHED to detect when a certain target has been reached and we want to move
* to the next one.
*
* We start as usual our periodic task:
*
* \snippet tutorials/Tutorial-PP_A.cpp T4-A
*
* Next we initialize all the relevant parameters related to the PROFILED_POSITION_MODE.
*
* Note that we are assuming that the motors defined in the XML have a device_id that goes from 0 to (NUM_MOTORS-1).
*
* The most noteworthy part of this setup is the one where we subscribe to the event identified as EVENT_PROFILED_POSITION_REACHED.
* Since it is an _asynchronous__ event, we will need to call the method spin() to actually execute the callback.
* Additionally, we ask each of the motors to move to the position 0.
*
* \snippet tutorials/Tutorial-PP_A.cpp T4-B
*
* As it can be seen in the definition of the callback, we simply wait for N = NUM_MOTORS events to be reached before incrementing the
* global variable PHASE.
*
* \snippet tutorials/Tutorial-PP_A.cpp T4-cb
*
* In the main loop we simply check if a new target position should be sent and we print on screen (independently from the PHASE)
* the actual position.
*
* This example alos illustrates as different flags (second argument of setProfiledPositionTarget) will influence the behaviour of the
* path generator.
*
* \snippet tutorials/Tutorial-PP_A.cpp T4-C
*
*/

/**
* \page tutorial-4b Profiled position B
*
* In this example we have a simple application that controls N motors in PROFILED_POSITION_MODE, where N is defined only through
* the XML.
*
* Contrariwise to the example \ref tutorial-4a , we are not making any hard-coded assumption about the device_id used
* in the XML or the number of motors.
* But similarly to that example, we use an __asynchronous__ subscription to EVENT_PROFILED_POSITION_REACHED to change the state
* of our application.
*
* In this case the method spin() is used to sleep a certain amount of time; in fact, when a timeout is specified, the application will
* block either until at least a single event is reached or untile the end of the timeout.
*
* Only a single motor will be moved at each time; the next motor in the list will move when the previous finished its motion.
*
* \snippet tutorials/Tutorial-PP_B.cpp T4B-A
*/
