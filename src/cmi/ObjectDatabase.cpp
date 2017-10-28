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

#include "cmi/ObjectDatabase.h"
#include <fstream>
#include <vector>

namespace CanMoveIt
{

std::ostream& operator<< (std::ostream &out, ObjectData const &data)
{
    out<< (ObjectData &)data; return out;
}

std::ostream& operator<< (std::ostream &out, DataStatus &status)
{
    switch( status){
        case DS_NO_DATA:   out << COLOR_RED    <<  "NO_DATA"   << COLOR_RESET; break;
        case DS_NEW_DATA:  out << COLOR_GREEN  <<  "NEW_DATA"  << COLOR_RESET; break;
        case DS_OLD_DATA:  out << COLOR_YELLOW <<  "OLD_DATA"  << COLOR_RESET; break;
        case DS_TIMEOUT:   out << COLOR_YELLOW <<  "TIMEOUT"  << COLOR_RESET; break;
    }
    return out;
}

std::ostream& operator<< (std::ostream &out, ObjectData &data)
{
    out <<  data.convert<std::string>();
    return out;
}


ObjectData::ObjectData( ObjectEntry* entry):
    _is_new_data ( DS_NO_DATA ),
    _raw_type ( entry->type() )
{
    switch( entry->type() )
    {
    case UINT8:   _data = (uint8_t) 0;
    case UINT16:  _data = (uint16_t) 0;
    case UINT32:  _data = (uint32_t) 0;
    case UINT64:  _data = (uint64_t) 0;

    case INT8:    _data = (int8_t) 0;
    case INT16:   _data = (int16_t) 0;
    case INT32:   _data = (int32_t) 0;
    case INT64:   _data = (int64_t) 0;

    case FLOAT32: _data = (float) 0;
    case FLOAT64: _data = (double) 0;

    case STRING:  _data = std::string("");

    default: throw std::runtime_error("Unhandled case");
    }
}

//------------------------------------------------------------------
class ObjectsDatabase::Impl
{
public:
    typedef std::vector< ObjectData >  DatabaseMap;

    DatabaseMap          object_database;
    ObjectsDictionaryPtr object_dictionary;
    RW_Mutex             od_mutex;
    Impl( ObjectsDictionaryPtr dictionary):  object_dictionary(dictionary) {}
};


ObjectsDatabase::ObjectsDatabase (ObjectsDictionaryPtr dictionary): _d( new Impl(dictionary) )
{
    rebuild();
}

void ObjectsDatabase::rebuild(ObjectsDictionaryPtr dictionary )
{
    if(dictionary)
    {
        _d->object_dictionary = dictionary;
    }
    uint16_t s = _d->object_dictionary->size();
    if(s==0)
    {
        throw std::runtime_error("EDS file probably corrupted");
    }

    _d->object_database.clear();
    _d->object_database.reserve( s );

    for (int i=0; i< s; i++)
    {
        ObjectEntry *entry = &( _d->object_dictionary->at(i) );
        ObjectData  obj( entry );
        _d->object_database.push_back( obj );
    }
}

ObjectsDatabase::~ObjectsDatabase()
{
    _d->object_database.clear();
    delete _d;
}



ObjectData& ObjectsDatabase::getData(ObjectKey const& key)
{
    return _d->object_database.at(key);
}


DataStatus  ObjectsDatabase::getValue(ObjectKey const& key, Variant *value )
{
    ScopedReadLock lock( &_d->od_mutex );
    ObjectData& obj = getData(key);

    DataStatus temp = obj.get_isnew();
    *value = obj.get();
    return temp;
}

void ObjectsDatabase::setValue(ObjectKey const& key, Variant const& value, TimePoint timestamp)
{
    ScopedWriteLock lock( &_d->od_mutex );

    ObjectData& obj = getData(key);
    obj.set( value , timestamp );
    obj.set_isnew ( DS_NEW_DATA );
}

uint8_t ObjectsDatabase::setValueFromBytes(ObjectKey const& key, const uint8_t *bytes, TimePoint timestamp)
{
    ScopedWriteLock lock( &_d->od_mutex );
    ObjectData& obj = getData(key);

    obj.get().copyFromBytes( bytes );
    obj.setTimestamp(timestamp);
    obj.set_isnew ( DS_NEW_DATA );
    return ( obj.size() );
}


} //end namespace











