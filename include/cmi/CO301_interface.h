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

#ifndef CAN301_IntERFACE_H_
#define CAN301_IntERFACE_H_

#include "cmi/log.h"
#include <stdlib.h>
#include <map>
#include "CAN_Interface.h"
#include "CO301_def.h"
#include "ObjectDictionary.h"


namespace CanMoveIt {

#define BIND(func_ptr) std::bind(func_ptr,this,_1, _2)

/** @defgroup CANopen L2: CANopen CiA 301 interface
 *  @brief This layer takes care of the implementation of the CANopen 301 layer (generic devices, not just motors).
 *  See CO301_Interface for details.
 */

/** @brief This structure is passed to the method pdoMapping to setup a certain PDO.
 *  It is a quick and dirty replacement of a std::container. We can't used directly a container
 * from STL to avoid binary compatibility problems.
 * It contains a list of objects to be mapped (identiefied using the index and subindex stored in the ObjectID).
 */
class PDO_MappingList
{
public:
    PDO_MappingList(): _size(0)           {}
    uint8_t size() const                    { return _size; }
    void  push_back(ObjectID const& obj ) { assert( _size<8); _placeholder[_size++] = obj;}
    ObjectID& operator[](uint8_t i)         { assert( i<_size); return _placeholder[i];     }
private:
    ObjectID _placeholder[8];
    uint8_t    _size;
};

class CO301_Interface;

/** Shared pointer to an instance of CO301_Interface. */
typedef std::shared_ptr<CO301_Interface> CO301_InterfacePtr;

/**
 * @ingroup CANopen
 * @brief A simple function to allocate easily an instance of the class CO301_Interface. Note that when the instance is created,
 * the application will silently try to read the object entry 0x1000 usign SDO.
 * If no reply is received, the applicotion will assume that the device is not connected and will trhow and Exception (std::runtime_error).
 *
 * @param can_port 		Pointer to the can port returned by the function canOpen().
 * @param node_id 		Node of the CAN device.
 * @param device_model	name of the device. It is associate to a certain object dictionary. Note that only one
 * 						instance of the ObjectDictionary is created for the entire process (Singleton-like).
 * @param device_id     A (possibly) unique identifier to recognize the instance of this particular device.
 * @return              The pointer to the newly created instance of CO301_Interface.
 */
CO301_InterfacePtr create_CO301_Interface(CANPortPtr can_port, uint8_t node_id,  const char* device_model, uint16_t device_id);


/** @ingroup CANopen
 *  @brief Get the pointer of a device previously allocated by the function cmi_Init or create_CO301_Interface.
 */
CO301_InterfacePtr get_CO301_Interface(uint16_t device_id);


/** @ingroup CANopen */
typedef std::map<uint16_t, CO301_InterfacePtr> DeviceList;

/** @ingroup CANopen
 *  @brief Get all the CO301_InterfacePtr available.*/
const DeviceList& cmi_getDeviceList();

/** @ingroup CANopen
 *  @brief Remove the allocate instance of CO301_Interface that correspond to a certain node_id.
 * @param node_id 		Node of the CAN device.
 * @return              0 is the object was found and removed, 1 if it was not found, -1 if an error occurred.
 */
int remove_CO301_Interface(uint16_t device_id);

/**
 *  @ingroup CANopen
 *  @class CO301_Interface
 *  @brief This class is the abstract interface to any general CANopen device that fulfill
 *  the specifications of CiA 301.
 *
 *  This class makes possible to easily interact with a single device (CNopen node) using the registers described
 *  by the ObjectDictionary.
 *  It stores internally an instance of ObjectDatabase that will store the most recent values received from the node.
 */

class CO301_Interface: public CanInterface
{
private:
    /** This method must be called once everytime the CANopen device is switched on. It will
     *  check if it is actually a CANopen device (reading the object entry 0x1000).
     */
    bool init();

    enum{ USE_DEFAULT_PDO_COBID = 0 };

public:
    /** Constructor. This is private because you must use the factory function create_CO301_Interface. */
    CO301_Interface( CANPortPtr can_port, uint8_t node_id, ObjectsDictionaryPtr obj_dict, uint16_t device_id);

    friend  CO301_InterfacePtr create_CO301_Interface(CANPortPtr can_port, uint8_t node_id,  const char* device_model, uint16_t device_id);

public:

    virtual ~CO301_Interface();

    uint8_t  node_ID() const;

    /** This function is used to launch a SDO upload (as it is known in CANopen).
     * In other words, it sends a message to the slave asking to modify the value of an object in the dictionary.
     * If the modification is accepted by the slave, the new value will be updated in the local ObjectDatabase (_asynchronously_).
     *
     * @param key   Use the ObjectKey or the ObjectID to identify the object entry.
     * @param value the new value to be sent to the device.
     */
    void sdoWrite( ObjectKey const& key , Variant const& value );
    void sdoWrite( ObjectID const& id , Variant const& value )
    {
        sdoWrite( findObjectKey(id), value);
    }

    /** This function will send a SDO download request (as it is known in CANopen).
     * It sends a message to the slave asking for the value of an object in the dictionary.
     * Once the reply is received (_asynchronously_), the local ObjectDatabase is updated and the associate
     * callbacks and events are executed.
     * Note that the reply may take some time to be received; this methos is non-blocking.
     *
     * @param key   Use the ObjectKey or the ObjectID to identify the object entry.
     */
    void sdoObjectRequest( ObjectKey const& key  );
    void sdoObjectRequest( ObjectID  const& id  )
    {
        sdoObjectRequest( findObjectKey(id));
    }

    /** Get the most up to date value of an object from the ObjectDatabase. This information is stored locally on the master side.
     * The value might be obsolete, since this method doesn't ask the slave to send new data.
     *  You can check the returned value to see if the data is new (i.e. it has been read for the first time) or not.
     *
     * @param key     Use the ObjectKey or the ObjectID to identify the object entry.
     * @param value   Value returned by the method.
     * @return
     * 		- DS_NEW_DATA means that is the first time you read this message (fresh data was received from the slave).
     * 		- DS_OLD_DATA means that this is not the first time you read this value. There wasn't any update in the midtime.
     * 		- DS_NO_DATA means that this value have never been received from the slave.
     */
    DataStatus getLastObjectReceived(ObjectKey const& key, Variant* value );

    /// More general template version of getLastObjectReceived(ObjectKey const& , Variant* );
    template <typename T> DataStatus getLastObjectReceived( ObjectKey const& key, T* value );

    /// See getLastObjectReceived(ObjectKey const& , Variant* );
    DataStatus getLastObjectReceived( ObjectID  const& id,  Variant* value )
    {
        return getLastObjectReceived( findObjectKey(id), value );
    }

    template <typename T> DataStatus getLastObjectReceived( ObjectID const& id, T* value )
    {
        return getLastObjectReceived( findObjectKey(id), value );
    }


    /** This function send a SDO download request and wait for the reply.
     * It is similar to sdoReadRemoteObject, but it is blocking.
     *
     * @param key                   Use the ObjectKey or the ObjectID to identify the object entry.
     * @param value                 Here we will store the value received by the slave.
     * @param wait_answer_timeout   Timeout in **microseconds**.
     * @return
     * 		- DS_NEW_DATA is the message was received.
     * 		- DS_NO_DATA if the timeout expired.
     */
    DataStatus sdoRequestAndGet( ObjectKey const& key,  Variant* value, Microseconds wait_answer_timeout);

    /// More general template version of getLastObjectReceived( ObjectKey const& ,  Variant*, Microseconds);
    template <typename T> DataStatus sdoRequestAndGet( ObjectKey const& key, T* value, Microseconds wait_answer_timeout);

    /// See getLastObjectReceived( ObjectKey const& ,  Variant*, Microseconds);
    DataStatus sdoRequestAndGet( ObjectID  const& id,   Variant* value, Microseconds wait_answer_timeout)
    {
        return sdoRequestAndGet( findObjectKey(id), value, wait_answer_timeout);
    }

    template <typename T> DataStatus sdoRequestAndGet( ObjectID const& id, T* value, Microseconds wait_answer_timeout)
    {
        return sdoRequestAndGet( findObjectKey(id), value, wait_answer_timeout);
    }

    /** Use this method to do the entire PDO mapping process.
     * Note that the device must be in state NMT_PRE_OPERATIONAL.
     *
     * @param pdo             Identifier of the PDO. See PDO_Id.
     * @param mapping_info    It is a list of ObjectID to be mapped into the 8 byte of the CAN frame associated to the PDO.
     */
    void pdoMapping(PDO_Id pdo, PDO_MappingList&  mapping_info, uint16_t new_cobid = 0);

    /** Set the transmission type to Synchronous. It means that the message is sent once every x SYNC messages
     * /a SYNC message is sent either usind cmi_sendSync() or CO301_Interface::sendSync.
     *
     * @param pdo             Identifier of the PDO. See PDO_Id.
     * @param num_of_syncs    Num of synchs to be received to trigger the transmission of the PDO.
     *
     */
    void pdoSetTransmissionType_Synch(PDO_Id pdo,uint8_t num_of_syncs = 1);

    /** Set the transmission type to Asynchronous.
     * The PDO is transmitted when an internal event happen inside the slave. This event can be either defined by the
     * manufacturer or be standardized by the CiA CANopen profile.
     * You should read the manual of your manufacturer to be sure about the actual behaviour of your device.
     * You can (and should) control the frequency at which the PDO is transmitted, in particular you should specify an
     * inhibit_time different that is not too low to prevent the continuous transmission of the message.
     *
     * @param pdo             Identifier of the PDO. See PDO_Id.
     * @param event_type      Either ASYNCH_PROFILE or ASYNCH_MANUFACTURER.
     * @param inhibit_time    Time to wait between two consecutive PDO transmissions. It accept multiples of 100 microseconds
     * @param event_time      If the PDO transmission (i.e. an event) is not triggered during this period of time,
     *                        generate a trigger. Zero to disable it.
     * */
    void pdoSetTransmissionType_ASynch(PDO_Id pdo, uint8_t event_type, Microseconds inhibit_time, Milliseconds event_time);

    /** Enable or disable a certain PDO.*/
    void pdoEnableComm(PDO_Id pdo, bool enable);

    /** Send a NMT message. See for reference NMT_States.*/
    void sendNMT_stateChange(NMT_StatesCmd state);

    /** Get the dictionary entry associated to a certain ObjectID or ObjectKey. */
    const ObjectEntry& getObjectDictionaryEntry (ObjectKey const& key);
    const ObjectEntry& getObjectDictionaryEntry (ObjectID  const& id)  { return getObjectDictionaryEntry( findObjectKey(id)); }

    /** Send a synch message.
     * @remark
     * Note that this method is provided only for debug purposes.
     * The synch is a kind of message that is not associated to a particular device, since it is received by all the
     * CAN nodes in the physical bus.
     * Use cmi_sendSync() instead.
     */
    void sendSync ();

    /** Wait the answer related to a certain objects. It will unblock if:
     * - the object was correctly updated by a SDO answer or PDO TX
     * - an error int he SDO upload was received.
     *
     * @param key          The identifier of the object you need to wait for.
     * @param timeout_rel  This is the wait timeout (infinite wait is forbidden !).
     * @return             Returns true if message was received within the specified time interval, false otherwise.
     */
    bool waitObjectUpdate(ObjectKey const& key,Microseconds timeout_rel);

    void push_PDO_RX(PDO_Id pdo, uint8_t msg_size, uint8_t* data);

    /** Find the ObjectKey that is pointing to a certain entry in the dictionary.
     *  Note that passing the ObjectKey instead of ObjectID to certain functions such as getLastObjectReceived
     * or sdoWrite is more efficient.
     * */
    ObjectKey findObjectKey( ObjectID id) ;
    ObjectKey findObjectKey(uint16_t ind, uint8_t sub = 0);

    /** This command will send a command that will trigger an anser of the device.
     * The answer will change the state of the variable OperationState. */
    void sendNMT_lifeguard();

    /// get the latest value of the operational state. You need first to send the command sendNMT_lifeguard().
    NMT_OperationalState getOperationalState();

    void setHeartbeatProducerPeriod( Milliseconds ms);

    /// To be used only after the invokation of reloadObjectDictionary.
    /// You can pass optionally a new dictionary. With no argument, re-use the previous one.
    void rebuildObjectDatabase(ObjectsDictionaryPtr new_dictionary = ObjectsDictionaryPtr());

    ObjectsDictionaryPtr getObjectDictionary();

    ObjectID getObjectID(ObjectKey key );

    ObjectKey tryFindObjectKey( ObjectID id) ;

private:

    bool SDO_Interpreter(const CanMessage & m);
    bool PDO_Interpreter(const CanMessage & m);
    int  receivedNewObject(ObjectKey const& key, const uint8_t * data, TimePoint timestamp);
    void initPDO(PDO_Id pdo);

    class Impl;
    Impl* _d;

};


template <typename T> DataStatus CO301_Interface::getLastObjectReceived( ObjectKey const& key, T* value )
{
    Variant val;
    DataStatus ret = getLastObjectReceived(key, &val );
    *value = val.convert<T>();
    return ret;
}

template <typename T> DataStatus CO301_Interface::sdoRequestAndGet( ObjectKey const& key, T* value, Microseconds wait_timeout)
{
    Variant val;
    DataStatus ret = sdoRequestAndGet(key, &val, wait_timeout );
    *value = val.convert<T>();
    return ret;
}



} // namespace CanMoveIt */

#endif /* CAN301_IntERFACE_H_ */
