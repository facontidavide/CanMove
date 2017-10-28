#ifndef DOXYCHAPTER1_H
#define DOXYCHAPTER1_H

/**
* \page chapter-1 Chapter 1: Architecture of the framework
* The architecture of CAN/MoveIt is organized in different Layers.
*
* __Layer 0: OS Abstraction__.
*
* Here we provides simple wrappers to OS dependent functions (threads, mutex, timers).
* Its purpose is to simplify the portability of the code on different Operative Systems.
* The library Boost (<www.boost.org>) was used but, to avoid potential conflict, it was wrapped into a different
* namespace (CMI instead of boost).
*
* __Layer 1: CAN interface__.
*
* In this layer the user can just send and receive raw CAN messages; no specific protocol is implemented.
* Layer 1 takes care, additionally, of the asynchonous FIFO mechanism and does the "dispatching" of callbacks to
* the subscribers when a CAN message is received, using a model similar to publish-subscribe.
*
* __Layer 2: CANopen main protocol (CiA 301)__.
*
* At this level all the details related to a certain protocol are implemented, to be specific the CANopen Cia 301.
* Note that there is no relationship between this layer and the concept of motors, motion control and servodrives.
* We just have the general purpose functionalities of CANopen such as __SDO write/read, PDO mapping, NMT, SYNCH, Emergency__ , etc.
*
* __Layer 3: Motor Abstract layer__.
*
* This layer provides an API that is specific for motion control. Instead
* of reinventing the wheel, the motion modes and the parameters defined by the CiA 402 standard are used:
*    - Profiled Position.
*    - Profiled Velocity.
*    - Interpolated Position.
*    - Torque.
*
* \subsection sub1 General rules of the layered architecture
* These are the main rules which were used to keep the software modular, decoupled and extendable:
* 1. Any layer uses the functions, classes and methods of the layers which are __lower__ in the hierarchy.
* 2. At the same time, any layer hides by default the layers which are __lower__ in the hierarchy (from the view point of the
* application developer).
* 3. Any layer is completely decoupled from the layers which are __higher__ in the hierarchy.
* 4. It is always possible to access functionalities in a layer lower than the current one.
*
* At first sight this may sound complicated but in fact it is really simple, just to make some examples:
* - If you use the API of Layer 2, in other words the class __CO301_Interface__, you __don't__ need to directly
* care about threads, mutexes and raw CAN messages. At the same time, you don't have any way to use to the functionalities
*   of the Motor Abstract Layer (__MAL__). This make sense, since many devices using the CiA 301 aren't motion controllers.
* - If you are using the MAL, you can still get access to the underlying instance of CO301_Interface, In this way
*   you can write and read directly a register (Object in the Dictionary) using the SDO method offered by CANopen.
*
* \subsection sub2 Simplified UML class diagram
*
* To get an idea of how classes and layers are connected to each other, take a look to the following
* <a href="http://en.wikipedia.org/wiki/Class_diagram">UML class diagram.</a>
*
* \image html canmoveit-layers.png
*
* If you are not familiar with UML, don't be afraid, we will summarize further the noteworthy points.
*
* 1. __CO301_Interface__ is inherithed from __CAN_Interface__.
* 2. __MAL_Interface__ just provides virtual methods. The actual implementation can be found in the derivative class
* __MAL_CANOpen402__.
* 3. It is possible to implement Interfaces which use _proprietary_ (__non CANopen__) protocols. In this case the custom
* interface must inherit from __MAL_Interface__. Such class will conceptually span between L2 and L3, because if implements
* a protocol (L2) speicific for Motion Controllers (L3).
* 4. Any instance of __CO301_Interface__ owns one and only one instance of __ObjectDatabase__.
* 5. Any instance of __ObjectDatabase__ refers to a certain device specific __ObjectDictionary__. Note that the same
* ObjectDictionary can be shared among multiple __ObjectDatabase(s)__.
* 6. For your information: __ObjectDictionary__ works, in fact, as a singleton: only one instance of it is created for each kind
* of device in your application.
*
*
*/

#endif // DOXYCHAPTER1_H
