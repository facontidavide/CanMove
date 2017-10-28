/**
\page basics-te Type Erasure

 *
* As we already know any Object Entry in the Dictionary of CANOpen contains its own type (most commonly
* unsigned and signed integers of different size and strings).
*
* Since we need to manipulate objects with different type, two different mechanisms of type erasing
* are provided in CAN/MoveIt.
*
*  - A very __strict__ one based on absl::any (http://www.boost.org/doc/libs/1_55_0/doc/html/any.html).
*  - a more __flexible__ , yet efficient, one called Variant, that is a simplified and modified version of
*    Poco::Variant (http://pocoproject.org/docs/Poco.Variant.html)
*
* In the following example we play with both these classes to familiarize with their differences and features.
*
* ----------------------------------------------------
*
* In the first section, we will create two instances of CMI::any, any_A and any_B.
* Both of them will remember the value that was used to initialize them __and__ the type.
* If we try to simply copy a CMI::any instance value into a variable, we will get an error
* at compilation time. In fact, implicit convertion is forbidden.
*
* The only way to "extract" the value of "any" is using an explicit cast funtion called CMI::any_cast.
*
* \snippet tutorials/test_any.cpp ANY-A
*
* You can only cast an any instance to its original type. Trying to cast it to something different, would
* cause an exception.
*
* \snippet tutorials/test_any.cpp ANY-B
*
* It should be noted also that:
*  - If we copy any_A into any_B, the latter takes value and type of the former.
*  - We can detect easily if CMI::any is empty.
*
* \snippet tutorials/test_any.cpp ANY-C
*
* CMI::any works more or less as a replacement of void*, nevertheless its
* casting is always safe, since we can not cast CMI::any to a type different from the original one (we can't
* say the same of static_cast, unfortunately).
*
* ----------------------------------------------------
*
* When we want to have a safe and intuitive way to convert data between different types, we should use instead
* Variant.
* This class performs explicit and implicit conversions between types, but only when such convertion doesn't
* corrupt the original value, in particular, an exception will be thrown in these cases:
*
* - An attempt to convert or extract from a non-initialized ('empty') Variant variable.
* - An attempt to convert a negative signed integer value to an unsigned integer type storage.
* - Overflow is prohibited; attempt to assign a value larger than the target numeric type size can accommodate results.
*
* Precision loss, such as in conversion from floating-point types to integers or from double to float on platforms
* where they differ in size (provided double value fits in float min/max range), is permitted.
*
* \snippet tutorials/test_any.cpp ANY-D
*
* Additionally, for integral and floating point numeric values, following operations are supported:
*
*  - '+', '+=', '-', '-=', '*', '*=' , '/' and '/='
*
* For integral values, following operations are supported:
*
* - prefix and postfix increment (++) and decement (--)
*
* \snippet tutorials/test_any.cpp ANY-E
*
* Variant also play well with std::string. In fact can be used to convert from/to string any integral and floating
* value.
*
* \snippet tutorials/test_any.cpp ANY-F
*

* Full Source code
* =======================
*
* \note You can download the most up-to-date version of this example
* <a href="http://code.canmoveit.com/CanMoveIt/canmoveit-starterkit/tree/master">here</a>
*/

/**
 * \page basics-tm Time management
 *
 *
 * CAN/MoveIt fully embraces the use of std::chrono (feature introduced in Cx011); we use the implementation provided
 * by boost::chrono.
 *
 * Whenever we need to refer to a certain instant in time or time duration, this API will used; someone might feel that
 * this solution is unecessary cumbersome, on the other hand it is very effective and it allows the application
 * developer to avoid potential bugs, most of the time at compilation time.
 *
 * Think for example to a function that accepts a timeout as one of its arguments; the usual "solution" would be to document
 * the API specifing the actual time unit (for instance microseconds) and maybe to add some suffix to its name such as ___timeout_us__.
 *
 * This approach doesn't prevent accidental "human errors" or wrong conversions from other units such as seconds or microseconds.
 *
 * For this reason , a time duration can be expressed only using well defined types whaich have an implicit conversion mechanism:
 *  - CanMoveIt::Seconds
 *  - CanMoveIt::Milliseconds
 *  - CanMoveIt::Microseconds
 *
 * The conversion between different units is implicit whenever it is safe and must be explicit when there could be any
 * ambiguity or truncation.
 *
 * \snippet tutorials/test_chrono.cpp CHR-A
 *
 * Furthermore, we have to realize that an instant in time (CanMoveIt::TimePoint) is __conceptually__ different from
 * a time duration, which consequently can be seen as the difference between two aboslute TimePoints.
 * Even if we can represent both easily with a sufficiently large integral or a floating point variable, they __do not have__ the
 * same meaning and for this reason we want to prevent __at compilation time__ that the programmer accidentally mix these two concepts.
 *
 * Let's consider for example the functions sleep_for and sleep_until.
 *
 * \snippet tutorials/test_chrono.cpp CHR-B
 *
 * \note You can download the most up-to-date version of this example
 * <a href="http://code.canmoveit.com/CanMoveIt/canmoveit-starterkit/tree/master">here</a>
 *
 * \include tutorials/test_chrono.cpp
 * */

/**
 * \page hello-can Send and receive RAW CAN messages.
 *
 * The purpose of this example is twofold:
 *
 * - To show the lowest layer of CAN/MoveIt, the one that sends and receives raw CAN messages using the class CanMoveIt::CANPort.
 * - To implement a very simple "device scanner", even if not the most recommendable implementation, since might change the state
 *   of the device.
 *
 * First of all we need to load the CAN driver (a dynamic library that provides an interface to our CAN driver) and to create an
 * instance of CANPort, that will be always accessed through the smart pointer CanMoveIt::CANPortPtr.
 *
 * \snippet tutorials/hello_can.cpp HELLO-A
 *
 * Next, we subscribe a callback to be executed when a message with a particular pattern is received.
 *
 * \snippet tutorials/hello_can.cpp HELLO-B
 *
 * The callaback simply looks like this:
 *
 * \snippet tutorials/hello_can.cpp HELLO-CB
 *
 * Last, we send a single CAN message that will reset all the slave device (NMT).
 * Each of these device, according to the CANopen standard, will additionlly transmit a CAN message with cobid 0x700 + node_id.
 *
 * \warning When we reset the communication on the slave devices usign NMT_RESET_COMM, these devices might switch
 * to the state PRE_OPERATIONAL. In that case, devices like Servo drives would stop performing their control loops.
 *
 * \snippet tutorials/hello_can.cpp HELLO-C
 *
 * \note You can download the most up-to-date version of this example
 * <a href="http://code.canmoveit.com/CanMoveIt/canmoveit-starterkit/tree/master">here</a>
 *

 * */

