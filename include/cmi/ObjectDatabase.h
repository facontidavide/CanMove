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
#ifndef OBJECT_DATABASE_H
#define OBJECT_DATABASE_H

#include "cmi/ObjectDictionary.h"

namespace CanMoveIt {

/** @ingroup can_open */
typedef enum {
    DS_OLD_DATA = 1,       ///< This data is valid but it has been read already previously.
    DS_NEW_DATA = 2,       ///< New (fresh) data
    DS_NO_DATA  = 0,      ///< The server haven't received yet any sample of the data.
    DS_TIMEOUT  = 3        ///< Timeout (returned by sdoRequestAndGet).
}DataStatus;

std::ostream& operator<< (std::ostream &out, DataStatus &status);


class ObjectData{

    struct{
        uint8_t  _is_new_data : 2;
        uint8_t  _raw_type    : 5;
    };
    Variant    _data;
    TimePoint  _timestamp;

public:

    ObjectData( ObjectEntry* entry);

    const Variant& get() const { return _data;}
    Variant& get()         { return _data;}

    TypeID   type()  const { return static_cast<TypeID>(_raw_type); }
    uint8_t  size()  const { return CanMoveIt::getSize( type() ); }

    DataStatus  get_isnew( ) const
    {
        return static_cast<DataStatus>( _is_new_data);
    }

    void set_isnew(DataStatus ds)
    {
        _is_new_data = ds;
    }

    void      setTimestamp(TimePoint t)   { _timestamp = t;}
    TimePoint timestamp() const           { return _timestamp;}

    template <typename T> T    convert() const { return _data.convert<T>();}
    template <typename T> void convert(T* out) const { *out = _data.convert<T>();}

    template <typename T> T    extract() const { return _data.extract<T>();}
    template <typename T> void extract(T* out) const { *out = _data.extract<T>();}

    template <typename T> void set(const T& value, TimePoint t = GetTimeNow() )
    {
        _data.safeAssign( value );
        _timestamp = t;
    }

   // template <typename T> T get() const;
};

std::ostream& operator<< (std::ostream &out, ObjectData const &data);
std::ostream& operator<< (std::ostream &out, ObjectData &data);
std::ostream& operator<< (std::ostream &out, ObjectData &data);

/**
 * @ingroup can_open
 * @class ObjectsDatabase
 * @brief This database contains the current value (if it was updated by the master) of each element in the object dictionary.
 * @remark
 *  - Contrariwise to ObjectsDictionary, we have one instance of ObjectsDatabase for each device in the system.
 *  - The value of this local database give to you the most up-to-date value of a certain object of the dictionary.
 *  If you want the value of the object to be refreshed, you must ask the slave to do an update, for example using CO301_Interface::sdoReadRemoteObject
 * */


class ObjectsDatabase
{

public:
    /**
     * you need to pass the pointer of an object dictionary.
     * The constructor will allocate one ObjectData for each ObjectEntry of the dictionary.
     */
    ObjectsDatabase(ObjectsDictionaryPtr dictionary);
    ~ObjectsDatabase();

    /** Since value is stored as Variant, you need to cast to the right type. */
    DataStatus getValue( ObjectKey const& key, Variant* value );

    /**  Change the value of an entry in the ObjectsDatabase.*/
    void  setValue(ObjectKey const& key, const Variant& value, TimePoint timestamp = GetTimeNow());

    /**  Change the value of an entry in the ObjectsDatabase using an array of raw bytes (little indian notation).*/
    uint8_t setValueFromBytes(ObjectKey const& key, const uint8_t *data_bytes, TimePoint timestamp = GetTimeNow());

    /**  Read the value of an entry in the ObjectsDatabase.*/
    ObjectData& getData(ObjectKey const& key);

    void rebuild(ObjectsDictionaryPtr dictionary = ObjectsDictionaryPtr());

private:

    class Impl;
    Impl* _d;
};

/**
 * @ingroup can_open
*/
typedef std::shared_ptr< ObjectsDatabase > ObjectsDatabasePtr;




} //end namespace

#endif // OBJECTDICTIONARY_H
