/**
\page tutorial-1 Get started with SDO

First of all, we need to load a CAN driver, i.e a shared/dynamic library that is distributed
together with CAN/MoveIt.

\snippet tutorials/Tutorial-SDO.cpp T1-A

--------------------------------
Next, we need to:
 - Open a CAN port.
 - Create an Object Dictionary. This is done automatically loading a file with extension .EDS.
 - create an instance of CO301_Interface.

\snippet tutorials/Tutorial-SDO.cpp T1-B

__Alternatively__ we can do these three operations using a configuration file.
This option is more flexible because you can modify your configuration without recompiling your application.

\snippet tutorials/Tutorial-SDO.cpp T1-C

--------------------------------
We want to modify the value of the Object entry defined as MODE_OPERATION (an entry that can be found in the CiA 402 interface).
The new value will be TORQUE_MODE (see the file CO402_def.h ).

\snippet tutorials/Tutorial-SDO.cpp T1-D

When MODE_OPERATION is modified, the servo drive will update a read-oly entry called MODE_OPERATION_DISPLAY.
This operation will be very fast, but still takes a time that is different from zero.

To read the value of the object entry MODE_OPERATION_DISPLAY, we should first send a request to the device.

\snippet tutorials/Tutorial-SDO.cpp T1-E

Most of the time (but not always) when we execute the method getLastObjectReceived, it will return CanMoveIt::DS_NO_DATA or
CanMoveIt::DS_OLD_DATA. This means that the master hasn't received yet the reply.

We can't even guarantee that the actual request has been sent, since sdoWrite isn't blocking (it just push a CAN
message into a queue, that is processed by CAN/MoveIt when the bus is ready).

If the value of the object is a "fresh" the returned value fould be CanMoveIt::DS_NEW_DATA.

We can wait few milliseconds and try again.

\snippet tutorials/Tutorial-SDO.cpp T1-F

Alternatively, we can use a blocking method that sends a request and wait for the reply. Note that __any__
blocking method in CAN/MoveIt includes a timeout.

* Full Source code
* -------------------------
* \include tutorials/Tutorial-SDO.cpp
*
*/
//--------------------------------------------------------------------------------------------------------------


/**
* \page tutorial-2 PDO mapping and events.
*
* First of all, we define a PeriodicTask. Remember that:
* - startHook is executed only once at the beginning.
* - updateHoook is executed periodically.
* - stopHook is executed only once, when the task is stopped (not used in this tutorial).
*
* \snippet tutorials/Tutorial-PDO.cpp T2-A
*
* Since most of the application logic is moved into MyTask, the resulting main is considerably shorter.
*
* We use the XML configuration file to do the initialization.
*
* \snippet tutorials/Tutorial-PDO.cpp T2-B
*
* ----------------------------------------------------
*
* The method startHook is used to do the necessary setup:
*
* - Get a pointer of the device (CO301_Interface).
* - Set the device state to PRE_OPERATIONAL (mandatory to do the mapping of PDO.
* - We map two objects into PDO1_TX.
* - We configure PDO1_TX to emit a message every time a SYNC is broadcasted by the master.
* - We create two callbacks: a __synchronous__ one that is called when POSITION_ACTUAL_VALUE is updated and
*  an __asynchronous__ one that is associated to CURRENT_ACTUAL_VALUE.
*
* It must be noted that we packed two objects in a single PDO message, nevertheless from the point of
* view of the event() API, the subscription to each of these objects in completely independent.
* Even if in this example the two events share the same callback (PrintCallback) we could have used two
* different callbacks, one for each event.
*
* \snippet tutorials/Tutorial-PDO.cpp T2-C
*
* Let's take a look to PrintCallback.
*
* \snippet tutorials/tutorials.h PrintCallback
*
* __event.info__ is a variable with type __CMI::any__, that is nothing more than a fancy (and typesafe) replacement
* of a void* (basic type erasure).
* It __must__ be casted to CanMoveIt::EventDataObjectUpdated.
*
* ----------------------------------------------------
*
* The updateHook() is pretty simple: we just send a SYNC message using the function cmi_sendSync().
*
* The __synchronous__ event associated to POSITION_ACTUAL_VALUE will be called by a separate thread,
* instead the __asynchronous__ one associated to CURRENT_ACTUAL_VALUE requires the method spin() to be called.
*
* The latter solution is usually preferable to avoid problems of thread safety, that can happen when a
*  callback modifies the data accessed another thread.
*
* \snippet tutorials/Tutorial-PDO.cpp T2-D
*/


