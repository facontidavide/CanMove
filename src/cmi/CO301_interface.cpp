/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *******************************************************/


#include <deque>
#include "cmi/CO301_interface.h"
#include "cmi/EventDispatcher.h"


namespace CanMoveIt {


// container that can be accessed globally
std::map<uint16_t, CO301_InterfacePtr> _cmi_device_list;


CO301_InterfacePtr create_CO301_Interface(CANPortPtr can_port, uint8_t node_id, const char* OD_name, uint16_t device_ID)
{
    if( _cmi_device_list.find( device_ID ) != _cmi_device_list.end() )
    {
        Log::CO301()->error("device_ID {} was used twice and MUST be a unique identifier", device_ID);
        throw std::runtime_error("device_ID was used twice and MUST be a unique identifier" );
    }

    std::map<uint16_t, CO301_InterfacePtr>::iterator it;
    for(it = _cmi_device_list.begin(); it != _cmi_device_list.end(); it++)
    {
        CO301_InterfacePtr device = it->second;
        if( can_port == device->can_port() &&  node_id  == device->node_ID() )
        {
            Log::CO301()->error("It is illegal to use the same node ID on a single bus: node {}", node_id);
            throw  std::runtime_error(" Illegal: two CAN device try to share the same NODE ID on the same bus");
        }
    }

    ObjectsDictionaryPtr od_ptr = getObjectDictionary( OD_name );

    if ( !od_ptr )
    {   // this is the first time you try to access this dictionary and it is not stored.
        std::string filename = OD_name;
        filename.append(".eds");
        createObjectDictionary(filename.c_str(), OD_name);
        od_ptr = getObjectDictionary( OD_name );
    }

    if ( !od_ptr )
    {
        Log::CO301()->error("{} is not recognized as a valid Object Dictionary", OD_name);
        return CO301_InterfacePtr() ;
    }

    CO301_InterfacePtr device( new CO301_Interface(can_port, node_id, od_ptr,device_ID) );
    device->init();
    // might throw

    _cmi_device_list.insert( std::make_pair( device_ID, device));
    return device;
}

CO301_InterfacePtr get_CO301_Interface( uint16_t device_id )
{
    if( _cmi_device_list.find( device_id ) ==  _cmi_device_list.end())
    {
        Log::SYS()->error("no instance of CO301_Interface using this device_id ({}) was found.", device_id);
        throw std::runtime_error("There is not a CO301_Interface instance with this device_id. Check your configuration" );
    }
    return  _cmi_device_list.at( device_id ) ;
}

const DeviceList& cmi_getDeviceList() { return _cmi_device_list; }

int remove_CO301_Interface(uint16_t device_id)
{
    std::map<uint16_t, CO301_InterfacePtr>::iterator it =  _cmi_device_list.find( device_id );
    if( it ==  _cmi_device_list.end())
    {
        // not found
        return 1;
    }
    else{
        _cmi_device_list.erase( it );
        return 0;
    }
}

//--------------------------------------------------------------
class CO301_Interface::Impl
{
public:
    class PDO_MappingCache
    {
    public:
        std::vector<ObjectKey>   object;
        uint16_t                   cob_id;
    };

    std::map<uint16_t,std::shared_ptr<PDO_MappingCache> >  pdo_list;
    typedef std::map<uint16_t,std::shared_ptr<PDO_MappingCache> >::iterator PDO_List_iterator;

    template <typename T> bool pdoRX_find_and_fill(ObjectID id, const T& value);

    uint32_t     msg_sent;
    uint32_t     bytes_expedited_transfer;

    Mutex      wait_mutex;
    uint8_t    node_id;

    std::deque<CanMessage> _recorded_configuration_msgs;

    NMT_OperationalState operational_state;
    ObjectsDictionaryPtr  object_dictionary_ptr;
    ObjectsDatabase       object_database;

    Impl(ObjectsDictionaryPtr obj_dict):
        bytes_expedited_transfer(0),
        operational_state( NMT_STATE_NOT_DEFINED),
        object_dictionary_ptr ( obj_dict ),
        object_database( obj_dict )
    {}
};

/** Get the node ID of the device. */
uint8_t  CO301_Interface::node_ID() const { return _d->node_id; }


//--------------------------------------------------------------


CO301_Interface::CO301_Interface( CANPortPtr can_port, uint8_t node_id, ObjectsDictionaryPtr obj_dict, uint16_t device_id):
    CanInterface( can_port,device_id, 0,  0),
    _d( new Impl( obj_dict) )
{
    Log::CO301()->info("--- creating CO301_Interface for node {} with devide_id {} ----", (int32_t) node_id, (int32_t) device_id );

    InterpreterCallback callback =  std::bind(&CO301_Interface::SDO_Interpreter, this, std::placeholders::_1);
    this->addReadInterpreter( callback );

    _d->bytes_expedited_transfer = -1;
    _d->node_id = node_id;

    callback =  std::bind(&CO301_Interface::PDO_Interpreter, this, std::placeholders::_1);
    this->addReadInterpreter( callback );
}

void CO301_Interface::rebuildObjectDatabase(ObjectsDictionaryPtr new_dictionary)
{
    printf("old size: %d\n", _d->object_dictionary_ptr->size() );
    if( new_dictionary )
    {
        _d->object_dictionary_ptr = new_dictionary;
    }
    _d->object_database.rebuild( _d->object_dictionary_ptr);
    printf("new size: %d\n", _d->object_dictionary_ptr->size() );
    this->init();
}

CO301_Interface::~CO301_Interface() { delete _d;}

ObjectKey CO301_Interface::findObjectKey( ObjectID id)
{
    return _d->object_dictionary_ptr->find( id.index(), id.subindex() );
}

ObjectKey CO301_Interface::findObjectKey(uint16_t ind, uint8_t sub )
{
    return _d->object_dictionary_ptr->find( ind,sub );
}

ObjectID CO301_Interface::getObjectID(ObjectKey key )
{
    return _d->object_dictionary_ptr->getEntry(key).id();
}

ObjectKey CO301_Interface::tryFindObjectKey( ObjectID id)
{
    try{
        return findObjectKey(id);
    }
    catch(std::runtime_error ) { return ObjectKey(0xFF); }
}

const ObjectEntry& CO301_Interface::getObjectDictionaryEntry (ObjectKey const& key)
{
    return _d->object_dictionary_ptr->getEntry(key);
}


void CO301_Interface::initPDO(PDO_Id pdo)
{
    int16_t pdo_map = 0;
    int16_t pdo_comm = 0;

    if( pdo < PDO1_TX)
    {
        pdo_map   = PDO1_RX_Map  + (uint16_t)pdo;
        pdo_comm  = PDO1_RX_Comm + (uint16_t)pdo;
    }
    else{
        pdo_map   = PDO1_TX_Map  + (pdo-PDO1_TX);
        pdo_comm  = PDO1_TX_Comm + (pdo-PDO1_TX);
    }

    // get the COB_ID and rewrite it if necessary.
    uint32_t pdo_cob_id = 0;
    Variant temp(0);
    try{
        ObjectKey pdo_cob_id_key = findObjectKey( pdo_comm, 1);
        if( sdoRequestAndGet ( pdo_cob_id_key , &temp, Milliseconds(100)) == DS_NEW_DATA)
        {
            pdo_cob_id = temp.extract<uint32_t>( );
        }

        uint32_t new_cob_id = (pdo_cob_id & 0xffffFF80) + node_ID();
        if( new_cob_id != pdo_cob_id)
        {
            sdoWrite( pdo_cob_id_key, new_cob_id);
        }

        std::shared_ptr<Impl::PDO_MappingCache> mc (new Impl::PDO_MappingCache);
        mc->cob_id = new_cob_id;
        // _d->pdo_list.insert(std::make_pair(pdo_comm, mc) );
        _d->pdo_list[pdo_comm] = mc;

        // lets take more information
        uint8_t num_elements = 0;
        if( sdoRequestAndGet( findObjectKey( pdo_map, 0), &temp, Milliseconds(100)) == DS_NEW_DATA)
        {
            num_elements = temp.extract<uint8_t>( );
        }

        mc->object.resize( num_elements );

        for (uint8_t s=1; s <= num_elements; s++)
        {
            uint32_t value = 0;
            if( sdoRequestAndGet( findObjectKey( pdo_map, s), &temp, Milliseconds(100)) == DS_NEW_DATA)
            {
                value = temp.extract<uint32_t>( );
            }
            mc->object[s-1] = findObjectKey( 0xFFFF & ( value>>16),  0xFF & ( value>>8));
        }
    }
    catch( std::runtime_error &)
    {
        // no problem
    }

}

bool CO301_Interface::init()
{
    Variant device_type = 0;
    sdoRequestAndGet( findObjectKey( 0x1000, 0), &device_type, Milliseconds(100));
    if( device_type == 0)
    {
        throw std::runtime_error("Cant communicate with drive");
    }

    try{
        uint32_t emergency_id = EMERGENCY + node_ID();
        sdoWrite( findObjectKey( 0x1014,0),  emergency_id);
    }
    catch( std::runtime_error& e)
    {

    }

    for(int i=0; i<16; i++)
    {
        initPDO( static_cast<PDO_Id>(i) );
    }
    return true;
}

NMT_OperationalState CO301_Interface::getOperationalState()
{
    return _d->operational_state ;
}

void CO301_Interface::sdoObjectRequest(const ObjectKey & key)
{
    const ObjectEntry& entry  = _d->object_dictionary_ptr->getEntry(key);
    ObjectData&  data         = _d->object_database.getData(key);

    if( data.get_isnew() != DS_NO_DATA  )
    {
        data.set_isnew( DS_OLD_DATA );
    }

    CanMessage msg;
    const uint8_t ccs = 2<<5;

    msg.cob_id = SDO_RX + node_ID();
    msg.len = 8;
    msg.data[0]= ccs ;
    msg.data[1]= entry.index() & 0x00FF;
    msg.data[2]= (entry.index() >> 8)& 0x00FF;
    msg.data[3]= entry.subindex();
    msg.data[4] = 0;
    msg.data[5] = 0;
    msg.data[6] = 0;
    msg.data[7] = 0;
    msg.wait_answer = NEED_TO_WAIT_ANSWER;
    msg.desired_answer = SDO_TX + node_ID();
    pushMessage(msg);
}


void CO301_Interface::sendNMT_stateChange(NMT_StatesCmd state)
{
    CanMessage msg;
    msg.len = 2;
    msg.cob_id = NMT;
    msg.data[0] = state;
    msg.data[1] = node_ID();
    msg.wait_answer = NO_WAIT;

    pushMessage(msg);
}

void CO301_Interface::sendNMT_lifeguard()
{
    CanMessage msg;
    msg.len = 0;
    msg.rtr = 1;
    msg.cob_id = NODE_GUARD + node_ID();
    pushMessage(msg);
}


void CO301_Interface::pdoMapping(PDO_Id pdo, PDO_MappingList& mapping_list , uint16_t new_cobid)
{
    int16_t pdo_map = 0;
    int16_t pdo_comm = 0;

    if( pdo < PDO1_TX)
    {
        pdo_map   = PDO1_RX_Map  + (uint16_t)pdo;
        pdo_comm  = PDO1_RX_Comm + (uint16_t)pdo;
    }
    else{
        pdo_map   = PDO1_TX_Map  + (pdo-PDO1_TX);
        pdo_comm  = PDO1_TX_Comm + (pdo-PDO1_TX);
    }

    if( _d->pdo_list.find(pdo_comm) == _d->pdo_list.end() )
    {
        Log::CO301()->warn("This PDO is not present {}", pdo);
        return;
    }

    //STEP 1: check the total size
    ObjectKey    obj_keys[8];
    uint8_t total_size = 0;

    for (int i=0; i< mapping_list.size(); i++)
    {
        obj_keys[i] = findObjectKey( mapping_list[i] );
        total_size += _d->object_dictionary_ptr->getEntry(obj_keys[i]).size();
    }
    if( total_size > 8)
    {
        throw std::runtime_error("size of mappable PDO exceeded");
    }

    // STEP 2: disable the PDO to start mapping
    ObjectKey pdo_comm_1 = findObjectKey( pdo_comm, 1) ;
    ObjectKey pdo_map_0  = findObjectKey( pdo_map,  0);

    if( new_cobid != 0)
    {
        _d->pdo_list[pdo_comm]->cob_id = new_cobid;
    }

    uint32_t cobid = _d->pdo_list[pdo_comm]->cob_id | (0x1 << 31);
    sdoWrite( pdo_comm_1 ,  cobid  );
    sdoWrite( pdo_map_0 , (uint8_t)0 );

    //STEP 3: map the objects
    _d->pdo_list[pdo_comm]->object.resize( mapping_list.size() );

    for (int i=0; i< mapping_list.size(); i++)
    {
        const ObjectEntry& e = _d->object_dictionary_ptr->getEntry(obj_keys[i]);
        uint32_t value = ((e.index() << 16) & 0xFFFF0000) + ((e.subindex() << 8) & 0xFF00) + (e.size()*8);

        sdoWrite( findObjectKey(pdo_map, i+1 ) , value );

        // save for future interpretation the keys
        _d->pdo_list[pdo_comm]->object[i] =  obj_keys[i] ;
    }
    //STEP 4: finish mapping
    sdoWrite( pdo_map_0 , mapping_list.size());
    cobid = _d->pdo_list[pdo_comm]->cob_id;
    sdoWrite( pdo_comm_1 ,  cobid  );

}

void CO301_Interface::pdoEnableComm(PDO_Id pdo, bool enable)
{

    int16_t pdo_comm = 0;

    if( pdo < PDO1_TX)
    {
        pdo_comm  = PDO1_RX_Comm + (uint16_t)pdo;
    }
    else{
        pdo_comm  = PDO1_TX_Comm + (pdo-PDO1_TX);
    }

    if( _d->pdo_list.find(pdo_comm) == _d->pdo_list.end() )
    {
        Log::CO301()->warn("This PDO is not present {}", pdo);
        return;
    }

    uint32_t new_cob_id = ( _d->pdo_list[pdo_comm]->cob_id );
    if (!enable)
    {
        new_cob_id |= (0x1 << 31) ;
    }
    sdoWrite( findObjectKey( pdo_comm,1) ,new_cob_id );
}


void CO301_Interface::pdoSetTransmissionType_Synch(PDO_Id pdo, uint8_t num_of_syncs)
{
    if( pdo < PDO1_TX)
    {
        throw RangeException("wrong input in pdoEnableCommTX");
    }
    uint16_t pdo_comm =  PDO1_TX_Comm + (pdo-PDO1_TX);

    pdoEnableComm(pdo, false);
    sdoWrite( findObjectKey( pdo_comm,2), num_of_syncs);
    pdoEnableComm(pdo, true);
}

void CO301_Interface::pdoSetTransmissionType_ASynch(PDO_Id pdo, uint8_t event_type, Microseconds inhibit_time, Milliseconds event_time)
{
    if( pdo < PDO1_TX) {
        throw RangeException("wrong input in pdoEnableCommTX");
    }
    uint16_t pdo_comm =  PDO1_TX_Comm + (pdo-PDO1_TX);

    pdoEnableComm(pdo, false);

    try{
        sdoWrite( findObjectKey( pdo_comm,2), (uint8_t)event_type );
    } catch(std::runtime_error) {};

    try{
        sdoWrite( findObjectKey( pdo_comm,3), (uint16_t)(inhibit_time.count()/100));
        Log::CO301()->warn("inhibit_time not supported by PDO 0x{0:X}", pdo_comm) ;
    } catch(std::runtime_error) {};

    try{
        sdoWrite( findObjectKey( pdo_comm,5), (uint16_t)(event_time.count()));
         Log::CO301()->warn("event_time not supported by PDO 0x{0:X}", pdo_comm) ;
    } catch(std::runtime_error) {};

    pdoEnableComm(pdo, true);
}

void  CO301_Interface::push_PDO_RX(PDO_Id pdo, uint8_t msg_size, uint8_t* data)
{
    if( pdo >= PDO1_TX)
    {
        throw RangeException("wrong input in push_PDO_RX");
    }

    int16_t pdo_comm;

    if( pdo < PDO1_TX)
    {
        pdo_comm  = PDO1_RX_Comm + (uint16_t)pdo;
    }
    else{
        pdo_comm  = PDO1_TX_Comm + (pdo-PDO1_TX);
    }

    if( _d->pdo_list.find(pdo_comm) == _d->pdo_list.end() )
    {
        Log::CO301()->error("This PDO is not present: {}", pdo);
        return;
    }

    CanMessage msg;
    msg.cob_id = _d->pdo_list[pdo_comm]->cob_id;
    msg.len = msg_size;
    memcpy( msg.data, data, msg_size);
    pushMessage( msg );
}


bool CO301_Interface::SDO_Interpreter(const CanMessage & m)
{
    bool recognized = true;

    if (m.getNode() != _d->node_id) { return false;}

    static uint8_t expedited_data[256];
    static int expedited_data_size = 0;
    static ObjectKey expedited_key;

    uint16_t COB = m.getCOB();
    TimePoint msg_tp = TimePoint() + Microseconds(m.timestamp_usec);

    switch (COB)
    {
    case SDO_TX:
    {
        uint8_t res = m.data[0];
        uint8_t scs = (res >> 5) & 0x7;
        uint8_t expedited_flag = (res >> 1) & 0x1;


        uint16_t index = ((m.data[2]<<8) & 0xFF00) +  m.data[1];
        uint8_t  subindex = m.data[3];
        ObjectKey key = _d->object_dictionary_ptr->find(index, subindex);

        if(scs == 4) // Abort SDO Transfer
        {
            uint32_t error_code =  m.data[4] + (m.data[5]<<8) +  (m.data[6]<<16) +  (m.data[7]<<24);

            EventData event;
            event.event_id   = EVENT_ERROR_IN_PROTOCOL;
            event.info       = error_code;
            event.timestamp  = GetTimeNow();
            events()->push_event( this->device_ID(), event );

            Log::CO301()->error("--error in the answer of the SDO: index [0x{:X}] subindex [0x{:X}]: {}\n",
                                (int)index ,  (int)subindex, m);
            std::string e;
            switch(error_code)
            {
            case 0x05040000: e.append(" toggle bit not alternated\n");                        break;
            case 0x05040001: e.append(" invalid client/server command\n");                    break;
            case 0x6010000 : e.append("Unsupported access to an object.\n");                  break;
            case 0x6010001 : e.append("Attempt to read a write-only object.\n");              break;
            case 0x6010002 : e.append("Attempt to write a read-only object.\n");              break;
            case 0x06020000: e.append("Object does not exist in the object dictionary\n");    break;
            case 0x06040041: e.append("Object cannot be mapped to PDO.\n");                   break;
            case 0x06040042: e.append("Number and length of objects to be mapped exceeds PDO length.\n");   break;
            case 0x06040043: e.append("General parameter incompatibility.\n");                break;
            case 0x06070010: e.append("Data: length of service parameter does not match\n");  break;
            case 0x06090011: e.append("Sub-index does not exist\n");                          break;
            case 0x06090030: e.append("Value range of parameter exceeded (only for write access).\n");    break;
            case 0x06090031: e.append("Value of parameter written too high.\n");              break;
            case 0x06090032: e.append("Value of parameter written too low.\n");               break;
            case 0x08000000: e.append("General error\n");                                    break;
            case 0x08000022: e.append("Data cannot be transferred to or stored in application due to present device state.\n");  break;

            case 0x0F00FFC0: e.append("Wrong NMP state");    break;
            case 0x0F00FFBF: e.append("Illegal Command");    break;

            default:{
                e.append(" error: "); // TODO append code
                Log::CO301()->error("error code: [0x{:X}]", error_code);
            }

            }
        }
        else if( scs == 1 || scs == 3) // Download SDO
        { // it was a DOWNLOAD. everything ok. nothing to do
            //your command has been accepted: store the value locally WITHOUT a callback
            receivedNewObject( key, & ( this->getLastMsgSent().data[4]), msg_tp );
        }
        else if( scs == 2 && expedited_flag) // UPLOAD Segment SDO
        {  // it is the answer of an UPLOAD!!
            //your command has been accepted: store the value locally WITH a callback.
            receivedNewObject( key, &(m.data[4]), msg_tp );
        }
        else if( scs== 0) // Initiate SDO Upload
        {	//initiate upload
            int reply_size = m.data[4];
            _d->bytes_expedited_transfer = reply_size;

            bool toggle = 0;
            CanMessage msg_out;
            msg_out.cob_id = SDO_RX + node_ID();
            msg_out.len = 8;
            msg_out.wait_answer  = NEED_TO_WAIT_ANSWER;
            msg_out.desired_answer = SDO_TX + node_ID();

            int req_num = reply_size / 7;
            if(reply_size > req_num*7) req_num += 1;

            toggle = (bool)(req_num%2);

            for (int i=0; i<req_num; i++)
            {
                if (toggle==1)   msg_out.data[0] = 0x60 ;
                else			  msg_out.data[0] = 0x70 ;
                toggle = !toggle;
                pushMessage(msg_out, true);
            }
            expedited_data_size = 0;
            expedited_key = _d->object_dictionary_ptr->find(index,subindex);
        }
        else if( _d->bytes_expedited_transfer > 0 )
        {
            int received_here = m.data[0] & 0x0F;
            if( received_here == 0) received_here = 7;
            for (int i=0; i<received_here; i++ )
            {
                expedited_data[ expedited_data_size + i] = m.data[1+i];
            }
            expedited_data_size += received_here;
            _d->bytes_expedited_transfer -= received_here;

            if( _d->bytes_expedited_transfer <=0 )
            {
                expedited_data[expedited_data_size] = '\0';
                receivedNewObject( expedited_key , expedited_data, msg_tp );
            }
        }
        else{
            Log::CO301()->error("SDO_TX... what is this? 0x{:X}\n{}", res, m);
        }

    }break;

    case 0x700:
    {
        switch( m.data[0] & 0x7F)
        {
        case NMT_STATE_PRE_OPERATIONAL: _d->operational_state = NMT_STATE_PRE_OPERATIONAL; break;
        case NMT_STATE_STOPPED:         _d->operational_state = NMT_STATE_STOPPED;         break;
        case NMT_STATE_OPERATIONAL:     _d->operational_state = NMT_STATE_OPERATIONAL;     break;
        default: Log::CO301()->error("unrecognized message NMT: 0x{:X}", m.data[0]);
        }
    } break;

    case EMERGENCY:
    {
        uint32_t error_code = m.data[0] + (m.data[1]<<8) + (m.data[2]<<16) + (m.data[3]<<24);

        if( (error_code & 0xFFFF) != 0)
        {

            Log::CO301()->error("Emergency with code 0x{:X} -> 0x{:X}",
                                 (error_code & 0xFFFF),
                                 ((error_code>>16) & 0xFFFF) );

            EventData event;
            event.event_id = EVENT_EMERGENCY_FAULT;
            event.info = error_code;
            event.timestamp = GetTimeNow();
            events()->push_event( device_ID(), event );
        }
        recognized = true;

    }break;

    default: recognized = false; break;

    } // end of switch

    return recognized;
}

bool CO301_Interface::PDO_Interpreter(const CanMessage & m)
{
    bool recognized = false;
    std::shared_ptr< Impl::PDO_MappingCache> pdo_mapped;

    for ( Impl::PDO_List_iterator it = _d->pdo_list.begin(); it != _d->pdo_list.end(); it++)
    {
        if( it->second->cob_id == m.cob_id )
        {
            pdo_mapped = it->second;
            break;
        }
    }

    if( pdo_mapped )
    {
        recognized = true;
        int array_offset=0;

        for(unsigned i=0; i< pdo_mapped->object.size(); i++)
        {
            ObjectKey key = pdo_mapped->object[i];
            TimePoint tp = TimePoint() + Microseconds(m.timestamp_usec);
            int size_obj = receivedNewObject(key,  &(m.data[array_offset]) , tp );
            array_offset += size_obj;
        }
    }
    return recognized;
}

int CO301_Interface::receivedNewObject(ObjectKey const& key, const uint8_t * data, TimePoint timestamp)
{
    int i = 0;
    {
       // LockGuard lock( _d->wait_mutex );
        LockGuard lock( _d->wait_mutex );

        // update the value inside the local storage
        i = _d->object_database.setValueFromBytes(key, data, timestamp );

        EventData event;
        event.timestamp = timestamp;

        const ObjectEntry& entry = _d->object_dictionary_ptr->getEntry(key);
        event.info = EventDataObjectUpdated( entry, _d->object_database.getData(key) );
        event.event_id = entry.id().get();

        // push event related to this object.
        // To be done before broadcast otherwise some functions will not work properly
        this->events()->push_event(device_ID(), event );
    }

    return i;
}

void CO301_Interface::sendSync ()
{
    CanMessage msg;
    msg.cob_id = SYNC;
    this->pushMessage( msg );
}

ObjectsDictionaryPtr CO301_Interface::getObjectDictionary() { return _d->object_dictionary_ptr; }

void CO301_Interface::sdoWrite(const ObjectKey & key, const Variant& value )
{
    const ObjectEntry& entry = _d->object_dictionary_ptr->getEntry(key);

    // we will use the built-in type conversions of ObjectData (actually Variant)
    // This must be a copy, not  a reference or pointer, since we don¡t want to modify the original
//    Variant t =  _d->object_database.getData(key).get() ;
//    t.safeAssign( value );

    if( entry.access_type() == ObjectEntry::RO || entry.access_type() == ObjectEntry::CNST)
    {
        Log::CO301()->error("object 0x{:X} / 0x{:X} can't be written. Check the access type in the dictionary.",
                             entry.index(), entry.subindex() );
        return;
    }
    CanMessage msg;
    const uint8_t CCS = 1<<5;
    const uint8_t expedited = 1<<1;
    const uint8_t indicated = 1;

    msg.cob_id = SDO_RX + node_ID();
    msg.len = 8;
    msg.wait_answer  = NEED_TO_WAIT_ANSWER;
    msg.desired_answer = SDO_TX + node_ID();

    msg.data[0]= CCS | ((4 - entry.size() )<<2) | expedited | indicated;
    msg.data[1]= entry.index() & 0x00FF;
    msg.data[2]= (entry.index() >> 8)& 0x00FF;
    msg.data[3]= entry.subindex();

    uint32_t vi = 0;

    if( isSigned( entry.type() ) )
    {
        int32_t temp = value.convert<int32_t>();
        vi = static_cast<uint32_t>(temp);
    }
    else{
        vi = value.convert<uint32_t>();
    }

    msg.data[4] =  vi & 0x00FF;
    msg.data[5] = (vi >>  8) & 0x00FF;
    msg.data[6] = (vi >>  16) & 0x00FF;
    msg.data[7] = (vi >>  24) & 0x00FF;

    pushMessage(msg);
}

void CO301_Interface::setHeartbeatProducerPeriod( Milliseconds ms)
{
    try{
        sdoWrite( ObjectID(0x1017,0), ms.count() );
    }
    catch(std::runtime_error &)
    {
        Log::CO301()->error("setHeartbeatProducerPeriod cant be used on devices"
                             " that dont have the object 0x1017 in the ObjectDictionary. ");
    }
}


bool CO301_Interface::waitObjectUpdate(ObjectKey const& key, Microseconds timeout_rel)
{
    //CHECK if the thread is different from the CANREAD thread, otherwise exception
    if( isCanReadThread())
    {
        throw std::runtime_error("You CAN NOT use the waitAnswer inside msgReceivedCallback. It would cause an infinite wait" );
    }

    if(timeout_rel > Microseconds::zero() )
    {
        TimePoint deadline = GetTimeNow() + timeout_rel;
        ObjectData& obj    = _d->object_database.getData( key );

        absl::Condition object_updated( +[](ObjectData* obj)
        {
            return obj->get_isnew() != DS_NEW_DATA;
        }, &obj );

        bool done = _d->wait_mutex.AwaitWithDeadline( object_updated, absl::FromChrono(deadline) );
        return done;
    }
    return true; //return true if condition has been signaled or it was already NEW_DATA
}


DataStatus CO301_Interface::getLastObjectReceived( ObjectKey const& key,
                                                   Variant* value )
{
    DataStatus ret = _d->object_database.getValue(key, value);
    if( ret != DS_NO_DATA)
    {
        _d->object_database.getData(key).set_isnew( DS_OLD_DATA);
    }
    return ret;
}


DataStatus CO301_Interface::sdoRequestAndGet( ObjectKey  const& key,
                                              Variant* value,
                                              Microseconds wait_answer_timeout)
{
    sdoObjectRequest(key);

    if( wait_answer_timeout > Microseconds::zero()  )
    {
        int ret = this->waitObjectUpdate((key), wait_answer_timeout );
        if ( ret == false )
        {
            return DS_TIMEOUT;
        }
    }
    return getLastObjectReceived ((key), value);
}

} // namespace CanMoveIt */


