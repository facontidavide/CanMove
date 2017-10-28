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


#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <signal.h>
#include "tinyxml2/tinyxml2.h"
#include "cmi/globals.h"
#include "cmi/CAN.h"
#include "cmi/MAL_Interface.h"
#include "cmi/CMI.h"
#include "cmi/MAL_CANOpen402.h"
#include "OS/AsyncManager.h"

#ifdef LINUX
#include <sys/file.h>
#endif

namespace CanMoveIt{

//----------------------------------------------------------------------

std::map<uint16_t, MAL_InterfacePtr> _cmi_motors_list;

const MotorList&  cmi_getMotorList()  { return _cmi_motors_list; }


void cmi_sendSync()
{
    //To get actual position and current from CanOpen drives (SMART), send a sync. PDO will be received
    CanMessage msg;
    msg.cob_id = SYNC; //don't need to touch other default values in the class CanMessage
    for( CANPortPtr can_port_handle: CMI::get().opened_can_ports )
    {
        can_port_handle->send( &msg );
    }
}


MAL_InterfacePtr cmi_getMAL_Interface( uint16_t device_id )
{
    extern std::map<uint16_t, CO301_InterfacePtr> _cmi_device_list;

    if( _cmi_motors_list.find( device_id ) ==  _cmi_motors_list.end())
    {
        Log::SYS()->error("no instance of MAL_Interface using this device_ID ({}) was found.", device_id );
        if( _cmi_device_list.find( device_id ) !=  _cmi_device_list.end())
        {
            Log::SYS()->error("... but an instace of CO301_Interface does exist! Have you forgotten the field <Motor> in the XML?" );
        }
        throw std::runtime_error("There is not a MAL_Interface instance with this device_id. Check your configuration" );
    }
    return _cmi_motors_list.at( device_id );
}


int cmi_init( char const* config_file)
{
    if( config_file != NULL )
    {
        if( cmi_loadFile( config_file ) < 0)
        {
            throw std::runtime_error("error parsing the configuration file");
        }
    }
    return 1;
}


bool cmi_configureMotor( MAL_InterfacePtr device_ptr, bool autostart, const char* filename )
{
    if( device_ptr->configureDrive( filename ) != SUCCESSFUL ) { return false; }
    if( autostart)
    {
        if( device_ptr->startDrive() != SUCCESSFUL) return false;
    }
    return true;
}


tinyxml2::XMLElement* getUniqueChild(const char* name, tinyxml2::XMLDocument* parent)
{
    tinyxml2::XMLElement* child = parent->FirstChildElement( name );

    if( !child )
    {
        Log::SYS()->error("XML: Cant find the element {} in root", name );
        throw std::runtime_error( std::string("XML: Cant find the element ") +  name );
    }
    if (child != parent->LastChildElement( name ))
    {
        Log::SYS()->error("XML: duplicated element {}", name );
        throw std::runtime_error( std::string("XML: duplicated element ") +  name );
    }
    return child;
}

tinyxml2::XMLElement* getUniqueChild(const char* name, tinyxml2::XMLElement* parent)
{
    tinyxml2::XMLElement* child = parent->FirstChildElement( name );

    if( !child )
    {
        Log::SYS()->error("XML: Cant find the element {} inside {}", name, parent->Name() );
        throw std::runtime_error( std::string("XML: Cant find the element ") +  name );
    }
    if (child != parent->LastChildElement( name ))
    {
        Log::SYS()->error("XML: duplicated element {} inside {}", name, parent->Name() );
        throw std::runtime_error( std::string("XML: duplicated element ") +  name );
    }
    return child;
}

template< typename T >
inline T convert(tinyxml2::XMLElement* element)
{
    if( element->GetText() == NULL)
    {
        throw std::runtime_error("error parsign the XML element %s", element->Name() );
    }

    std::istringstream iss( std::string( element->GetText() ) );
    T obj;

    iss >> std::ws >> obj >> std::ws;

    if(!iss.eof()) throw std::runtime_error("error parsign the XML element %s", element->Name());
    return obj;
}

template< typename T >
inline T convert(tinyxml2::XMLAttribute* element)
{
    if( element->Value() == NULL)
    {
        throw std::runtime_error( std::string("error parsign the XML element ") + element->Name() );
    }

    std::istringstream iss( std::string( element->Value ) );
    T obj;

    iss >> std::ws >> obj >> std::ws;

    if(!iss.eof()) throw std::runtime_error( std::string("error parsign the XML element ") + element->Name());
    return obj;
}


int cmi_loadFile( const char* filename )
{
    extern std::map<uint16_t, CO301_InterfacePtr> _cmi_device_list;

    bool init_failed = false;
    // Create an empty property tree object
    using namespace tinyxml2;

    Log::SYS()->info("Trying to read configuration file {}", filename) ;

    XMLDocument doc;
    if( doc.LoadFile( filename ) )
    {
        Log::SYS()->error("Cant load file {}", filename );
        throw std::runtime_error( std::string("Cant load file ") + filename);
    }


    std::map<std::string, std::string> can_map;

    XMLElement* el_can_ports = getUniqueChild("CanPorts" , &doc);

    for( XMLElement*  child = el_can_ports->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if( strcmp(child->Name(), "CanPort") != 0)
        {
            Log::SYS()->error("Group <CanPorts> should have only elements named <CanPort>" );
            throw std::runtime_error("Group <CanPorts> should have only elements named <CanPort>" );
        }

        const char* portname  = child->Attribute("portname");
        const char* bitrate   = child->Attribute("bitrate");

        if( portname && bitrate)
        {
            can_map.insert( std::pair<std::string, std::string>( portname,  bitrate ) );
        }
        else{
            Log::SYS()->error("XML: Missing attribute in <CanPort>. Must have [portname] and [bitrate]");
            throw std::runtime_error("XML: Missing attribute in <CanPort>");
        }
    }

    //---------------------------------------------------------

    XMLElement* el_obj_dict = getUniqueChild("ObjectDictionaries" , &doc);

    for( XMLElement*  child = el_obj_dict->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if( strcmp(child->Name(), "ObjectDictionary") != 0)
        {
            Log::SYS()->error("Group <ObjectDictionaries> should have only elements named <ObjectDictionary>") ;
            throw std::runtime_error("Group <ObjectDictionaries> should have only elements named <ObjectDictionary>" );
        }

        const char* file   = child->Attribute("file");
        const char* name   = child->Attribute("name");

        if( file && name)
        {
            createObjectDictionary( file, name );
        }
        else{
            Log::SYS()->error("XML: Missing attribute in <ObjectDictionary>. Must have [file] and [name]");
            throw std::runtime_error("XML: Missing attribute in <ObjectDictionary>");
        }
    }

    //---------------------------------------------------------
    XMLElement* el_devices = getUniqueChild("Devices" , &doc);

    //for each children of <Devices>...
    for( XMLElement*  device = el_devices->FirstChildElement("Device"); device; device = device->NextSiblingElement())
    {

        unsigned device_ID;
        unsigned node_id;

        if( getUniqueChild("device_ID",  device)->QueryUnsignedText( &device_ID ) )
        {
            Log::SYS()->error("XML: Problem parsing the value of node <device_ID>");
            throw std::runtime_error("XML: Problem parsing the value of node <device_ID>");
        }
        if( getUniqueChild("can_node_ID", device)->QueryUnsignedText( &node_id ) )
        {
            Log::SYS()->error("XML: Problem parsign the value of node <can_node_ID>");
            throw std::runtime_error("XML: Problem parsign the value of node <can_node_ID>");
        }

        if( node_id < 1 ||  node_id > 127 )
        {
            Log::SYS()->error("XML: device_ID= {} the <can_node_ID> must be >=1 and <=127 ",device_ID);
            throw std::runtime_error("XML: the <can_node_ID> must be >=1 and <=127");
        }

        //---------------------------------------------------------
        std::string can_portname ( getUniqueChild("can_portname", device)->GetText() );
        if( can_map.find( can_portname ) == can_map.end() )
        {
            Log::SYS()->error("XML: can't find in the XML any CanPort with this portname:  ",  can_portname);
            throw std::runtime_error( std::string("XML: can't find in the XML any CanPort with this portname: ") + can_portname);
        }
        std::string can_bitrate = can_map[can_portname];
        CANPortPtr  can_port = openCanPort( can_portname.c_str(), can_bitrate.c_str() );

        //---------------------------------------------------------
        std::string dictionary_name ( getUniqueChild("dictionary_name", device)->GetText() );
        ObjectsDictionaryPtr obj_dict;
        CO301_InterfacePtr device_ptr;

        obj_dict = getObjectDictionary( dictionary_name.c_str()  );
        if( !obj_dict )
        {
            Log::SYS()->error("XML: <dictionary_name> cant be loaded: ", dictionary_name);
            throw std::runtime_error( std::string("XML: [dictionary_name] cant be loaded: ") + dictionary_name);
        }

        try{
            device_ptr = create_CO301_Interface( can_port , node_id, dictionary_name.c_str() , device_ID  );
            // the previous line might throw...

            //---------------------------------------------------------
            XMLElement* motor = device->FirstChildElement("Motor");
            // Motor subgroup is optional

            if( motor )
            {
                unsigned max_current = 10000;

                if( motor->FirstChildElement("max_current") )
                {
                    unsigned max_current;
                    motor->FirstChildElement("max_current")->QueryUnsignedText( &max_current );
                }

                MAL_InterfacePtr motor_ptr = MAL_InterfacePtr( ( new  MAL_CANOpen402( device_ID, device_ptr, max_current ) ) );

                // encoder_resolution, invert_direction and gear_reduction are all used to calculate [device_info->rad_to_encoder]
                double rad_to_encoder  = 1.0 / ( 2.0 * 3.141592654 );


                if( motor->FirstChildElement("encoder_resolution") )
                {
                    double encoder_resolution;
                    motor->FirstChildElement("encoder_resolution")->QueryDoubleText( &encoder_resolution );
                    rad_to_encoder *=  encoder_resolution;
                }

                if( motor->FirstChildElement("gear_reduction") )
                {
                    double gear_reduction;
                    motor->FirstChildElement("gear_reduction")->QueryDoubleText( &gear_reduction );
                    rad_to_encoder *=  gear_reduction;
                }

                if( motor->FirstChildElement("invert_direction") )
                {

                    std::string isReversed ( motor->FirstChildElement("invert_direction")->GetText( ) );

                    if( isReversed.compare( std::string("true" ) ) == 0 )
                    {
                        rad_to_encoder *= -1.0f;
                    }
                    else            {
                        if( isReversed.compare( std::string("false" ) ) != 0 )
                        {
                            Log::SYS()->error("XML: <invert_direction> must be either \"false\" or \"true\" ");
                            throw std::runtime_error("XML: <invert_direction> must be either \"false\" or \"true\" ");
                        }
                    }
                }
                motor_ptr->setRadToEncoder( rad_to_encoder );
                _cmi_motors_list.insert( std::make_pair( device_ID,  motor_ptr ) );
            }
        }
        catch(...)
        {
            init_failed = true;
            Log::SYS()->error("Can't detect device_ID: {} with node_id: {}", device_ID, (int)node_id );
            CanMoveIt::sleepFor( Milliseconds(100) );
        }
    }
    if( init_failed )
    {
        throw std::runtime_error("at least one of the devices is not responding");
    }
    return _cmi_device_list.size();
}

} //end namespace
