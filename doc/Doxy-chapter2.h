#ifndef DOXYCHAPTER2_H
#define DOXYCHAPTER2_H

/**
* \page chapter-2 Chapter 2: Introduction to CANopen
*
* \tableofcontents
* Few words before we start...
* =========================
* The following documentation is not meant to be an exaustive description of CAN and CANopen, but only
* a short introduction for newbies that want to quickly understand the core concept of the CANopen protocol.
* All the details that are strictly related to the implementation of the CAnopen itself will not be discussed,
* because our software, CAN/MoveIt, is meant to take care of them for you.
*
* - \subpage chapter-2-1
* - \subpage chapter-2-2
* - \subpage chapter-2-3
* - \subpage chapter-2-4
* - \subpage chapter-2-5
*/

//--------------------------------------------------------------------------------------------------------------------

/**
* \page chapter-2-1 What is CAN and CANopen?
*
* __CAN__ (Controller Area Network) is a serial bus system, which was originally developed for automotive
* applications in the early 1980s.
* From the user's prospective, CAN provides only two communication services: the sending of a message
* (data frame transmission) and the requesting of a message (remote transmission request, RTR).
* All other services such as error signaling, automatic re-transmission of erroneous frames are user-transparent,
* since the CAN controller chip automatically performs these services.
*
* \image html CAN_wiring.png
*
* The advantages of the CAN physical layer are:
*
* - __Only two cables__ are needed to send and receive CAN frames.
* - __Daisy chain connection__ will reduce the amount of cabling in many multi-node applications.
* - It is a __broadly used__ standard, therefore many microcontrollers and System On Chips include a CAN controller.
*
* The CAN data link layer provides:
*
* -  __Multi-master capability and broadcast__: any CAN node may send a message and all messages transmitted are
* received by the other nodes in the network. All receiving nodes decide if they like to accept this message or not.
* -  __Error detecting mechanisms__ and re-transmission of faulty messages.
* -  __Non-destructive bus arbitration__: if two or more CAN nodes request simultaneously a message transmission,
* the message with the highest priority will get bus access immediately.
*
* The CAN frame
* -------------------------
* Any CAN frame consists of:
*
* -  __A 11 bit header__ (optionally 29 bits headers are allowed, even if it is not very common). CAN controllers
* can be configured to filter out some incoming messages with certain headers.
* -  __Between 0 and 8 byte of data__.
*
* Any manufacturer can easily create his own communication protocol based on this simple frame structure, but this
* would make interoperability impossible and would considerably increase the amount of work of system integrators.
*
* CANopen to the resque
* -------------------------
* CANopen is an higher level protocol on top of the CAN data link layer. This means that it provides a
* standardized interface that simplify and unify the way we communicate between devices. To make an analogy we may say that:
*
* - The CAN physical layer is the equivalent of __pen and paper__.
* -  CAN data link layer is like the entire set of __latin characters__.
* -  CANopen communication layer  is the __English grammar__.
* -  You application is a __novel__.
*
* \image html Canopen_or_not.jpg
*
* A special word shall be said about the  CANopen application layer; we might be tempted to compare it to a __dictionary__
* using the previous analogy, but instead of a large, monolithic dictionary, we would have many modular ones. In fact,
* the <a href="http://www.can-cia.org/index.php?id=systemdesign-profiles" target="_blank">application profiles</a>
* always focus on a specific kind of device such as motion controllers, I/O modules, lift control systems,
* medical devices, encoders, etc.
*
* In CANopen a single network can have up to __127 nodes__; most of the time, CANopen uses the 7 least signficant bits
* of the 11 bit header of the CAN frame to target a specific node of the network.
* In CANopen this 11-bit idis known as communication object identifier, or__ COB-ID__. In case of a transmission collision,
* the bus arbitration used in the CAN bus allows the frame with the smallest id to be transmitted first and without a delay.

*/
//--------------------------------------------------------------------------------------------------------------------

/**
* \page chapter-2-2 Objects And Dicitionaries
*
* The core of any CANopen node is the __Object Dictionary__, a sort of lookup table or, if you prefer, a list of registers;
* a single object is identified by a 16-bit index and a sub-index.
* Additionally, any object has the following attributes:
*
* - A __data type__, for example signed and unsigned integers of 8, 16 and 32 bits, float, date, time of day, etc....
*   These types can be standard or custom (i.e. defined by the manufacturer of the device).
* - If it is PDO mappable or not (we will see later what PDO and mapping means).
* - If it can be accessed as __Read-Only, Write-Only__ or __Read-Write.__
*
* For example the object known as  "device type" (which is mandatory in any CANopen device) has the following attributes.


Name of the object entry | Index   | Sub-index | Data Type  | Access   | Mappable
:------                  | :-----: | :-------: | :-------:  | :------: | :------:
Device Type              | 1000h   | 0h        | UNSIGNED32 | RO       | no


* In CMI/Core you manipulate and access Objects Entries of the dictionary as described further.
*
* ObjectID
* -------------------------
*
* Any object can be uniquelly identified by a index + sub-index pair; such pair is stored in the class ObjectID.
* \code{.cpp}
* ObjectID device_type( 0x1000, 0x0); //index 1000h, subindex 0h
*
* std::cout << "index: "    << device_type.index()    << std::endl;
* std::cout << "subindex: " << device_type.subindex() << std::endl;
* \endcode
*
* ObjectEntry, ObjectKey, ObjectDictionary
* -------------------------
* Any CANopen node in the network can potentially have its on Object Dictionary.
*
* In CAN/MoveIt and device has a pointer to a certain __ObjectDictionary__, that can be shared by several
* devices if those devices are identical. You can generate a dictionary loading the EDS file provided by the manufacturer.
*
* ObjectDicitionary is nothing more than a searchable list of instances of ObjectEntry; the latter class is
* the one that stores the attributes of a certain Object of the dictionary. Note that the information provided
*  by the ObjectEntry is constant
*
* \code{.cpp}
* ObjectsDictionary my_dictionary;
* // create dynamically an Object Dictionary from a standard file.
* my_dictionary.parseEDS("my_device.eds");
*
* // This intermediate identifier is used as an efficient way to access a certain entry of the dictionary several times.
* ObjectKey key = my_dictionary.find( 0x1000, 0x0 );
*
* const ObjectEntry& entry = my_dictionary.getEntry( key );
*
* std::cout << "index: "    << device_type.index()    << std::endl;
* std::cout << "subindex: " << device_type.subindex() << std::endl;
* std::cout << "size in bytes: " << entry .size()     << std::endl;
*
* // See the definition of TypeID for the list of supported types.
* TypeID my_type = entry.type();
* assert( my_type == T_uint32_t );
* \endcode
*
* ObjectDatabase
* -------------------------
* If we consider the role of the ObjectDictionary, we can notice that rather than data,
* it actually stores meta-data, i.e. some constant attributes related to a certain CANopen Object.
* This meta-data is constant and will never change during the execution of the program.
*
* Nevertheless, Objects and entries are meant to be the vehicle to access a certain "register" of the
* CANopen device and therefore we are interested to access (read and/or write) the actual value stored
* inside that register, in other words its data.
*
* The actual data associated to an object can be found in the class __ObjectDatabase__. Contrariwise to the
* ObjectDicitionary that is shared by several nodes/devices, we need an instance of ObjectDicitionary for
* each device that the Master is trying to control.
*
* I must be noted that the local ObjectDatabase stored on the Master side is just a __local cache__ where the
* last values received from the slave are stored; being a cache we can access the value several times, but we
* have to be carefully and check if the value is obsolate or not; in the next chapters we will see two mechanisms
* to send/receive data to/from an Object: SDO and PDO.
*
* In this section we will not show any code example that shows the API associated to this class; in fact we will
*  rarely use this class directly, since it functionalities are wrapped behind an higher level interface, that
* will be described in the next section: \ref chapter-2-3 .
*/


//--------------------------------------------------------------------------------------------------------------------


/**
* \page chapter-2-3 Interface to CANopen CiA 301 profile
*
* In this section we will learn how to use the class __CO301_Interface__ of the CMI/Core library.
*
* The Master application will need to create/allocate a single instance of this class for each node (slave)
* that needs to be controlled. The constructor will need the following arguments:
*
* - __CANPort__: the interface to the CAN driver.
* - __node_ID__: the identifier used by the slave device on the CANopen network.
* - __object_dictionary__: a reference to the object dictionary related to that device (see next page).
* - __device_ID__: a unique identifier used by the Master that must be unique and is not related to node_ID
*   (read further to understand why).
*
* Example: create two instances of CO301_Interface
* -------------------------
*
* In this example we want to control two devices, both connected to the same CAN network with a bitrate of 1 Mbit.
*
* One node has a __node_ID__ 31 and the other one 32. From the point of view of the master each of them will be
* identified using an alternative number, the __device_ID;__ this number will uniquely identify the corresponding slave.
*
* \code{.cpp}
* // Load the driver of the CAN hardware interface
* // It is a dynamic library (included with CAN/MoveIt).
*
* LoadCanDriver("libsocketcan.so" );
*
* //Open the CAN port on you master with the correct name and baudrate.
*  CANPort *can_port = canOpen("can0", "1M");

* // Here we create an instance of an object dictionary of our device
* // using an EDS file provided by the manufacturer.
* // In this case it is a servo drive of Ingenia, model "Pluto"
* CreateObjectDictionary("IngeniaPluto.eds", "pluto" );
*
* // We want to control two nodes of the network: the 31 and the 32.
* CO301_InterfacePtr device_A = create_CO301_Interface( can_port, 31, "pluto", 0 );
* CO301_InterfacePtr device_B = create_CO301_Interface( can_port, 32, "pluto", 1 );
* \endcode
*
*
* Alternatively, we can use an XML file to do the same configuration in a more flexible way
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
        <can_node_ID>31</can_node_ID>
        <can_portname>can0</can_portname>
        <dictionary_name>pluto</dictionary_name>
    </Device>

    <Device>
       <device_ID>1</device_ID>
        <can_node_ID>32</can_node_ID>
        <can_portname>can0</can_portname>
        <dictionary_name>pluto</dictionary_name>
    </Device>
</Devices>
* \endcode
*
* And on the source code side:
*
* \code{.cpp}
* LoadCanDriver("libsocketcan.so" );
*
* // this function will create two instances of CO301_Interface
* cmi_init("Pluto.xml")
*
* // we can find the correct pointer using the device_ID (see the xml")
* CO301_InterfacePtr device_A = get_CO301_Interface( 0 );
* CO301_InterfacePtr device_B = get_CO301_Interface( 1 );
* \endcode
*
* Even if it is good practive to use the same number for the device_ID and node_ID, it should be noticed
* that sometimes this is NOT possible.
*
* In fact, sometimes there are CAN hardware interfaces with more than one single port; this means that it
* would be possible to have the same node_ID repeated twice but on different networks; this makes impossible
* to use node_ID as the unique identifier of a non-trivial Master.
*
* For example the following configuration would be valid (two slaves with node 32 but located on two
* different networks):
*
* \code
* <CanPorts>
*     <CanPort portname="can0" bitrate="1M" />
*     <CanPort portname="can1" bitrate="1M" />
* </CanPorts>
*
* <ObjectDictionaries>
*     <ObjectDictionary file="IngeniaPluto.eds"  name="pluto"/>
* <ObjectDictionaries>
*
* <Devices>
*     <Device>
*         <device_ID>0</device_ID>
*         <can_node_ID>32</can_node_ID>
*         <can_portname>can0</can_portname>
*         <dictionary_name>pluto</dictionary_name>
*     </Device>
*
*     <Device>
*         <device_ID>1</device_ID>
*         <can_node_ID>32</can_node_ID>
*         <can_portname>can1</can_portname>
*         <dictionary_name>pluto</dictionary_name>
*     </Device>
* </Devices>
* \endcode
*
*/

//--------------------------------------------------------------------------------------------------------------------

/**
* \page chapter-2-4 Read and write objets using SDO
*
* CANopen provides a simple mechanism to allow communication between nodes of the network: the __Servide Data Object__ (SDO).
*
* \image html SDO1.jpg
*
* The main principles of SDO are:
*
* - It is a peer-to-peer communication that involves only two nodes.
* - We define as __SDO client__ the node that initiates the communication sending the fist frame; a __SDO server__ will always
*   send a message with a reply. In CAN/MoveIt the client is always the Master and the server is always the other nodes of
*   the networks (slaves).
* - The length of an SDO message is always 8 bytes.
* - The __SDO Upload__ is a request to read from a specific Object Dictionary entry. In othe words, this is what the Master uses
*   to __read__ the value of an object from a slave.
* - The __SDO Download__ is a request done by the client (our Master) to __write__ a value into a specific Object Dictionary
*   entry of a slave.
*
* When the Object to be read/written has a size of at most (4 bytes) 32 bits, we use what is called the __Expeditet SDO__
* protocol. In other words, only a single client request and a single server reply are sent. CAN/MoveIt also support
* Normal SDO, that is used for objects with a size larger than 4 bytes.
*
*
* For your information this is the way information is pacjed into the 8 bytes of the CAN frame

| SDO Download (write)         |      Byte 0       | Bytes 1-2 |  Byte 3  |   Bytes 4-7        |
| :---------------             | :-------------:   | :----:    | :------: | :------:           |
| __Request:__ master -> slave | command specifier | index     | subindex | DATA               |
| __Reply:__ slave -> master   | command specifier | index     | subindex | 0 or ERROR code    |
*
*
| SDO Upload (read)            |      Byte 0      | Bytes 1-2 |  Byte 3  |   Bytes 4-7        |
| :---------------             | :-------------:  | :----:    | :------: | :------:           |
| __Request:__ master -> slave |command specifier | index     | subindex | Not used           |
| __Reply:__ slave -> master   |command specifier | index     | subindex | DATA or ERROR code |


* When we want send consecutive SDO Uploads and/or Downloads to the same node, we  should keep in mind that it is forbidden to
* send a new SDO request until the previous reply was received.
* On the other hand, we are allowed to initiate a SDO request/reply cycle with other nodes of the system.
*
* A naïve implementation of the SDO protocol would block waiting for the reply of the client every time the master sends
* a reply; this behaviour will be called further "__blocking__"; in other software domains this mechanism is also known
* as "synchronous I/O", but since the word "synchronous" will be used in other contexts of the CMI/Core API,
*  we will not use that term for the time being.
*
* A __non-blocking__ framework can clearly be more efficient, especially when several slaves are connected. CMI/Core
* provides a mechanism to handle SDO either in a blocking or non-blocking way.
*
* Instead of digging into the details of the SDO protocol (the ID of the frame, the command specifier, how to deal with
* protocols other than the expedited), let's remember that CAN/MoveIt was designed to simplify your development process.
*
* SDO Download (Write)
* -------------------------
*
* In CAN/MoveIT a SDO Download is called  __sdoWrite __(as it ought to be); by default it is a non-blocking behaviour,
* since most of the time we don't really care about the actual confirmation. This means that:
*
* - The method __CO301_Interface::sdoWrite__ is non blocking. The actual request will not take place immediately but it
*  will be pushed into a FIFO queue that the CMI/Core framework will mange under the hood.
* - SDO replies are received and translated under the hood.
* - The blocking method __CO301_Interface::waitQueueEmpty__ can be used to be sure that all the pushed messages have
* been sent to the slave.
* - When there is an error in the SDO protocol, a log message is displayed; alternatively you can subscribe a callaback
* that will be executed when an error related to the SDO protocol take place (we will read about callbacks and events later).
* - A __timeout__ takes place after a certain amount of time when the SDO reply is not received by the master. This timeout
* generally means that the network is not working properly at the physical level.
* - The class takes care of checking __type consistency__ at run-time. If for example you try to write a negative number
* into an unsigned Object Entry or a value larger than 255 into an 8 bit integer, an exception will be launched.
*
* SDO Upload (Read)
* -------------------------
*
* The case of SDO Upload (i.e. reading data from a slave) it is slightly more complicated because it need to take into
* account different use cases. We will use the word "Request" in the API to refer to the action of sending a SDO Upload
* request message from the master to the slave.
*
* When the SDO reply is received, the CMI/Core framework will automatically process it and update the ObjectDatabase. In
* other words, the most recent value is stored on the master side and can be retrieved using the method __getLastObjectReceived.__
*
* In this simple example we wait some time to be sure that such reply was actually received; we can confirm that it is
* a new value reading the variable __ret__, that is ether NO_DATA, OLD or NEW.
*
* \code
*
    // Let's use a readable Object Entry of the Cia 402 profile
    ObjectID mode_operation_display( 0x6061, 0 );

    // device is CO301_InterfacePtr, i.e. a pointer of CO301_Interface
    device->sdoObjectRequest( mode_operation_display);

    // wait. 10 ms is very conservative value.
    sleep_for(Milliseconds(10));

    // This entry is a signed, 8bit integer.
    int8_t current_mode;
    DataStatus ret;
    ret = device->getLastObjectReceived( mode_operation_display, &current_mode);
*
* \endcode
*
* A smarter way to confirm that we have NEW data available on our local ObjectDatabase is to sobstitute the command __sleep_for__ with:
*
* \code
*
    // wait at most 10 milliseconds.
    // sleep_for(Milliseconds(10));
    bool received_before_timeout = device->waitObjectUpdate( mode_operation_display, Milliseconds(10) );
*
* \endcode
*
* There is also a _blocking_ method that condens these intermediate steps into a single one:
*
* \code
*
    // Instead of...
    //device->sdoObjectRequest( mode_operation_display);
    //device->waitObjectUpdate( mode_operation_display, Milliseconds(10) );
    //DataStatus ret = device->getLastObjectReceived( mode_operation_display, &current_mode);

    //Just use...
    device->sdoRequestAndGet( mode_operation_display, &current_mode, Milliseconds(10) );
*
* \endcode
*
* Last but not least, you can even create a callback associated to a certain object. his means that every time a certain
* Object Entry is updated on the local ObjectDatabase, a callback is executed. This method will be explained later in more details.
*
*
*/

//--------------------------------------------------------------------------------------------------------------------

/**
* \page chapter-2-5 NMT, PDO mapping and SYNC
*
* NMT (Network Management)
* -------------------------
*
By sending specific CAN messages (NMT messages), the master can control the state of the other nodes  of a CANopen network, i.e. it can change the state of all nodes or of an individual node by a single command. The possible states are:

<img class="aligncenter size-full wp-image-481" alt="NMT-States" src="http://www.can-moveit.com/wp-content/uploads/2014/03/NMT-States.jpg" width="389" height="229" />

The state influences the allowed operations of the node. In particular, these are the functionalities that are enabled in each state:


|              | Initializing | Pre-operational | Operational| Stopped  |
| :---         | :----------: | :----:          | :------:   | :------: |
| Boot-up      | yes          | -               | -          | -        |
| SDO          | -            | yes             | yes        | -        |
| PDO tx/rx    | -            | -               | yes        | -        |
| PDO mapping  | -            | yes             | -          | -        |
| Emergeny     | -            | yes             | yes        | -        |
| SYNC / Time  | -            | yes             | yes        | -        |
| Hearbeat / Nodeguard| -     | yes             | yes        | yes      |


Most of the time everything you will need to do in your application is either to call:
*
* \code
*
//given a pointer to CO301_interface with name "device"

device->sendNMT_stateChange(NMT_PRE_OPERATIONAL); // disable operation
//or
device->sendNMT_stateChange(NMT_OPERATIONAL); // enable operation.
*\endcode
*
* NMT messages are transmitted with the highest priority message identifier (CAN-ID 0).
* All nodes of a network are jointly addressed with the virtual node-ID 0; in this way, for example,
* all nodes can be set to "Operational" state at the same time for the sake of a simultaneous start of operation.
*
* \code
cmi_broadcastNMT_stateChange(NMT_OPERATIONAL); // enableoperation
*\endcode
*
* PDO (Process Data Object)
* -------------------------
*
*  CANopen distinguishes two types of PDOs. Keep in mind that the naming of CANopen always take the point of
*  view of the server (i.e. the slave).
*
* - Transmit Process Data Objects (<b>TPDOs</b>): PDOs produced by a node; in our case, this is a message
    sent by a __slave__ to the __master__.
* - Receive Process Data Objects (<b>RPDOs</b>): PDOs consumed by a node; in our case, this is a message
    sent by the __master__ to a __slave__.
*
* In order to use PDOs with a certain node,we must configure two sets of parameters:
*
* - The __communication parameters__, which are mainly  used in TPDOs to tell the producer (the slave) when and
*   how often messages must be transmitted.
* - The __mapping parameters__, which tell to the slave which objects values are stored inside a transmitted or
*   received message.
*
*
* PDO mapping
* -------------------------
*
* The basic idea between PDOs is that we can configure them to store how many Object Dictionary Entry that we
* want, as long as the total size of the message doesn't exceed 8 byte. This is called "mapping".
*
* To make an example based on profile CiA 402, a _single_ TPDO message can be mapped with these thee Object Entries:


Position actual value (0x6064)| Current actual value (0x6078)| Status Word (0x6041)
 :----------:                 | :------:                     | :------:
INT32 (4 bytes)               | INT16 (2 bytes)              | UINT16 (2bytes)


* As it can be seen, this mechanism is much more efficient than using thee times a SDO request. The advantage in
* term of bandwidth and delay will be even more obvious once we see how the communication parameter can be configured
* to transmit the PDO message automatically or on a periodic basis.
*
*\warning Remember that mapping can be done only when the slave is in pre-operational state!!
*
* In CANopen the PDO mapping can be done using multiple SDO Download commands in a certain order. It isn't too difficult
* but it can be a little cumbersome and error prone for beginners; luckily, CAN/MoveIt provides a simple way to do it:
*
* \code
*
    // In this example, we want to map some Object Entries of the CiA 402 profile into PDO1_TX.
    // we store these values in a vector-like structure of type PDO_MappingList.
    PDO_MappingList objects_to_map;

    objects_to_map.push_back( ObjectID(0x6040, 0) ); //POSITION_ACTUAL_VALUE
    objects_to_map.push_back( ObjectID(0x6078, 0) ); //CURRENT_ACTUAL_VALUE
    objects_to_map.push_back( ObjectID(0x6041, 0) ); //STATUSWORD

    // Do the magic! "device" is an instance of CO301_InterfacePtr
    device->pdoMapping( PDO1_TX,  objects_to_map);
*
* \endcode
*
*
* CMI/Core will check for you that:
*
* - each of these objects can actually be mapped according to the loaded EDS file and
* - the total message doesn't exceed 8 bytes.
*
* Additionally, when a transmitted message (in the example PDO1_TX) is received by the master, the CMI/Core
* framework will automatically demultiplex the content of the message and update the corresponding antries
* of the local ObjectDatabase; it will also trigger the connected callbacks, if any.
*
* What about sending a RPDO (i,e, commans from the master to a slave) ? In this case CMI/Core takes care of
* "serialize" the message for you.
*
* \code
*
    // In this example, we want to map two Object Entries of the CiA 402 profile into PDO1_RX.
    ObjectID profile_target( 0x607A, 0);
    ObjectID profile_velocity( 0x6081, 0);
    //... mapping process at the beginning....

    PDO_MappingList objects_to_map;
    objects_to_map.push_back( profile_target );
    objects_to_map.push_back( profile_velocity );
    device->pdoMapping( PDO1_RX,  objects_to_map);

    //... later (when the node is OPERATIONAL) and given a pointer to CO301_interface with name "device" ...

    int32_t  pos     = 1234; // the target position
    uint32_t max_vel = 100;  // the maximum velocity of the profile

    bool ret;
    ret = device->push_PDO_RX( PDO1_RX, profile_target, pos );
    //no message was sent! PDO1_RX is still incomplete (we miss one entry).
    assert(!ret);

    ret = device->push_PDO_RX(PDO1_RX, profile_velocity, max_vel);
    // Message is ready and will be sent.
    assert (ret);
*
* \endcode
*
<h2> PDO communication mechanisms (applicable to TPDO only)</h2>
There are basically two way to trigger a TPDO.

<h3>Synchronous</h3>
* TPDO transmission is triggered by a __SYNC__. In particular, the transmission takes place once every __N__ received SYNCs.
*
* It must be noted that SYNC is a message that is broadcasted by the Master to all the connected nodes; therefore
* a single SYNC (that can be sent using the command __cmi_sendSync()__ ), may allow the master to receive data from
* all of the connected slaves, if those were previously configured to send a TPDO.
*
* \code
*
    // The PDO mapping of PDO1_TX and PDO2_TX have been done already.
    // Tell to the slave to send the PDO1_TX and PDO2_TX messages once for every single SYNC message.
    device->pdoSetTransmissionType_Synch(PDO1_TX, 1);
    device->pdoSetTransmissionType_Synch(PDO2_TX, 1);

    // ...later...

    // The SYNC message will trigger both TPDOs.
    cmi_sendSync();
*
* \endcode
*
<h3> Asynchronous</h3>
* The TPDO is transmitted automatically every time a certain internal event takes place on the slave side. This event might
* be specific of the CANopen profile (in this case we will use the flag __ASYNCH_PROFILE__) or defined by
* the manufacturer (__ASYNCH_MANUFACTURER__).
*
* One of the most common events condition is simply "send the message if value of one of the mapped objects changed",
* nevertheless you should check if that is the case with the manufacturer of your slave device.
*
* To control how often this transmission will happen, we should use two additional parameters:
*
* - __Inhibit time__: the message can NOT be sent more often than a certain period of time. In other words, at
* least __X__ milliseconds should have passed since the last transmission before a new one takes place. This is particularly
* important to prevent that a certain TPDO associated to a parameter that may change continuously (for instance "current actual value")
*  collapse the CAN network.
* - __Event time__: this parameter is used to tell to the node that _even if the internal event didn't take place_ a transmission
* shall be done at least once every  __X__ milliseconds.
*
The syntax is simply:
*
* \code
*
    Milliseconds inhibit_ms(5); //not more often than 5  milliseconds.
    Milliseconds event_ms(20);  //at least once every 20 milliseconds.
    device->pdoSetTransmissionType_Asynch( PDO1_TX, ASYNCH_PROFILE, inhibit_ms, event_ms );
*
* \endcode
*
*
*
*/

#endif // DOXYCHAPTER2_H
