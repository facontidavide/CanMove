
/**

\page Tutorials

\note You can download the most up to date version of these tutorials
<a href="http://code.canmoveit.com/CanMoveIt/canmoveit-starterkit/tree/master">here</a>

* 1. Basics of the framework
* ========================
*
* 1.1 Type erasure

* -------------------------
* \subpage basics-te
*
* In this short example we will see the two main mechanisms of type erasure in CAN/MoveIt:
*
*  __CMI::any__ is a container that can hold any type. It forbid any kind of conversion, either implicit or
*  explicit. It can be seen as a safe alternative of void*, since the latter has an undefined behaviour
*  when you try to cast between types that are different from each other.
*
* __CanMoveIt::Variant__ on the other hand is a swiss army knife, but limited to few types (numbers and std::string).
* It allows to do implicit or explicit type conversions in a safe and efficient way.
* It prevents unexpected data loss, when performing narrowing or signedness conversions of numeric data types.
*
*
* 1.2 Time management
* -------------------------
* \subpage basics-tm
*
* We introduce a wrapper of boost::chrono (the same API of std::chrono) that CAN/MoveIt uses
* to manipulate __any__ information related to time, in particular timestamps and timeouts.
* Even if the resulting API might look more verbose than necessary, the implicit time conversion makes sure
* that you never use the wrong units in you application.
* For instance, you will __not__ be able to set accidentally a timeout argument in seconds instead of microseconds.
*
* 1.3 "Hello CAN"
* -------------------------
* \subpage hello-can "Hello CAN"
*
* In this short example we show the \ref chapter-1 "Layer 1" of communication, i.e how to read/write raw CAN messages.
* We are assuming that the devices connected on the CAN bus are CANopen.
*
*
* 2. The CANopen layer (CiA 301)
* ========================
*
* 2.1 SDO
* -------------------------
\subpage tutorial-1
*
* In this example you will:
 - Connect to a single device. We will compare a manual setup of the node with the script based one.
 - Read and write an Object in the dictionary using SDO.
 - We will see the difference between blocking and non-blocking read (i.e. SDO Upload).

* 2.2 PDO and callbacks
* -------------------------
\subpage tutorial-2
*
* The purpose of this tutorial is to introduce three main concepts:
* - Set the state of the device to either operational or pre-operational using NMT messages.
* - How to map a PDO, in particular a PDO_Tx (sent from the device to the master).
* - A simple example of the event callbacks and the difference between synchronous and asynchronous ones.
*
* 3. Motor Abstract Layer (CiA 402)
* ========================
*
* \note All of these examples will work only if your servo drive was correctly tuned to perform torque, velocity,
* and/or position control. CAN/MoveIt doesn't provide any "auto-magical-tuning" of you servo+motor+load system.
* Most of the time the manufacturer of the servodrive provides a graphical tool that will help you.
*
*
* 3.1 Interpolated Position
* -------------------------
* \subpage tutorial-3
*
* This example show how to use Interpolated Position in a periodic loop.
*
* 3.2 Profiled Position (periodic loop)
* -------------------------
*
* \subpage tutorial-4a
*
* This example show how to use Profiled Position in a periodic loop.
* We will use the loop to display the current position and velocity of the motor.
*
*
* 3.3 Profiled Position (no loop)
* -------------------------
*
* \subpage tutorial-4b
*
* This example show how to use Interpolated Position. It is similar to the previous one, but the state transition
* is purely based on asyncrhonous events.
*/



