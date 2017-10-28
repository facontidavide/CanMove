/**
 * \mainpage Introduction
 *
 *
 * CAN/MoveIt (__CMI__ in short) is a software library that can be used to easily interact with devices which use
 * the CANopen communication protocol.
 * It will be typically used to design a master controller which interacts with multiple slaves in real time.
 * CAN/MoveIt implements mainly two subsets of the CANopen standard.
 *
 * - __CiA 301__, that is the standard protocol that all the CANopen compatible devices share.
 * - __Cia 402__ that is specifically designed to control servo drives and motion controllers.
 *
 * A large part of CMI is dedicated to the control of servo drives and motor, nevertheless the implementation
 * of the generic CiA 301 stack is completely decoupled from the concept of motors and motion control and it can
 * be easily reused and extended to support more protocols.
 *
 * Our target audience are people which purchased a servo drive based on CANopen and need
 * to develop applications which control those drives and motors.
 * More specifically these are our goals:
 *
 * - Users which want to control servo drives __don't need to learn__ CANopen. They can use what we call
 *   the Motor Abstract Layer.
 * - CMI configures the servodrives in a way that is effective in 90% of the applications. If you are not happy
 *   with the default configuration, you can easily change it at compile time or run-time.
 * - If you want, you can access the CANopen Cia 301 layer directly. You can even send and read raw CAN messages.
 * - Multiple CANopen devices can be controlled at the same time. You can mix at the same time devices and servodrives
 *   from different manufacturer and CMI will take care of the tiny differences between them in terms of protocol and
 *   functionalities.
 *
 * Our goal is to make your life easier and your time to market shorter.
 *
 * <a href="mailto:faconti@icarustechnology.com">Let us know</a> if there is any feature that you need to be
 * implemented on CAN/MoveIt and we will be happy to help you.
 *
 */


