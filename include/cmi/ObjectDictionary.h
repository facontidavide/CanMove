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

#ifndef OBJECTDICTIONARY_H
#define OBJECTDICTIONARY_H

#include <string>
#include <sstream>
#include <map>
#include <typeinfo>
#include <boost/serialization/strong_typedef.hpp>
#include "builtin_types.hpp"
#include "variant.hpp"
#include "CAN.h"

namespace CanMoveIt {


class  ObjectData;
class  ObjectsDatabase;
class  ObjectEntry;


BOOST_STRONG_TYPEDEF(uint16_t, ObjectKey );


const uint32_t OD_SEG = 0x1000<<8;

/**
 * @ingroup can_open
 * @class ObjectID
 * @brief This is just than a index / subindex pair.
 * It is used to access objects inside ObjectsDictionary and ObjectsDatabase.
 * */


class ObjectID
{
public:
    explicit ObjectID( ) { _key = 0; }
    explicit ObjectID( const uint16_t & index, const uint8_t & subindex )
	{
		_key =(( index << 8) & 0xFFFF00) + subindex;
	}

    uint32_t operator ()() const                { return _key; }
    uint32_t get() const                        { return _key; }
    uint16_t index() const                      {return (_key>>8) & 0xFFFF;}
    uint8_t  subindex() const                   {return _key & 0xFF;}
    bool operator== (ObjectID const& k) const {return ( _key == k.get()) ;}
    ObjectID& operator= (ObjectID const& k)   {( _key = k.get()) ; return *this; }
    bool operator < (ObjectID const& k )const {return ( _key < k.get()) ;}
    bool operator > (ObjectID const& k )const {return ( _key > k.get()) ;}
private:
    uint32_t  _key;
};



/**
 * @ingroup can_open
 * @class ObjectEntry
 * @brief This is the container of a single entry inside a certain ObjectsDictionary.
 * */
class ObjectEntry
{

public:
    /**
     * @brief Defines if a certain Object entry can be read/written.
     * Note that this information is constant, and **MUST NEVER** change during the execution of the program.
     */
    typedef enum{
        RO =1,
        WO =2,
        RW =3,
        CNST = 0
    }AccessType;

    ObjectEntry();
    ObjectEntry(uint16_t index, uint8_t subindex,
                AccessType access, bool mappable,
                TypeID type,
                const char* description = NULL,
                Variant default_val = Variant());

    TypeID      type()            const;
    uint8_t     size()            const;
    ObjectID    id()              const;
    uint16_t    index()           const;
    uint8_t     subindex()        const;
    bool        PDO_is_mapable()  const;
    AccessType  access_type()     const;
    bool        empty()           const;
    const char* name()            const;
    void print() const;
    Variant   default_value() const;

private:

    struct Impl;
    std::shared_ptr< Impl > _d;
};


/**
 * @ingroup can_open
 * @class ObjectsDictionary
 * @brief The object dictionary is a collection of entries (defined by ObjectEntry).
 * @remark
 *  - __IMPORTANT__: usually you don't have to care about it. CO301_Interface takes care of everything transparently.}
 *  -  Note that only __one instance__ for each device type exists. Multiple devices of the same type share the same ObjectDictionary.
 * For example if you have two motors, one from Maxon and the other from Ingenia, there will be one (and only one) object dictionary
 * for each of them, doesn't matter how many physical motors are connected to the network.
 * */
class ObjectsDictionary
{
public:
    ObjectsDictionary();
    ~ObjectsDictionary();

    enum{ NOT_FOUND = 0xFFFF };

    const ObjectEntry& getEntry(ObjectKey const& key);
    ObjectKey find(uint16_t index, uint8_t subindex);

    void generateCode(const char* name);
    void parseEDS(std::ifstream &fin);

    uint16_t size() const;
    ObjectEntry& at( uint16_t position);
    void clear();

	friend class ObjectsDatabase;

    uint32_t vendorNumber() const;
    uint32_t productNumber() const;
    uint32_t revisionNumber() const;

protected:
    /** Insert a new object into the dictionary. This is done only during allocation of derived classes (lazy initialization). */
    void insert(ObjectEntry const&  obj);

private:
    struct Impl;
    Impl* _d;
};

/// Shared pointer of a ObjectsDictionary class.
typedef std::shared_ptr< ObjectsDictionary > ObjectsDictionaryPtr;

/**
 * @ingroup can_open
 * Use this function to create an ObjectDictionary instance using an EDS file as input.
 * Note that the function takes care that only one instance (identified by OD_name) for each object dictionary is created.
 * Bevertheless, if you use different OD_name and the same EDS file, you are still able to (erroneusly) allocated
 * multiple instances of the same ObjectDictionary.
 * This isn't a serious error (your software will run correctly anyway), but you are wasting memory.
 *
 * @param filename   Name of the file to be parsed (EDS file format).
 * @param OD_name    This name will be used to identify in the future the object dictionary. It is used for example
 *                   by GetInstanceObjectDictionary and create_CO301_Interface.
 * @return           The smart pointer of the ObjectDictionary (to be used for example in the constructors of ObjectDatabase and
 *                    CO301_Interface).
 **/
ObjectsDictionaryPtr createObjectDictionary(const char* filename, const char* OD_name);

/**
 * @ingroup can_open
 * Access pre-allocated ObjectDictionary. Don't forget that multiple ObjectDatabase instances can (and should)
 * share the same ObjectDictionary if the model of the devices is the same.
 * @param OD_name     Identifier of the OD.
 * @return            The smart pointer of the ObjectDictionary (to be used for example in the constructors of ObjectDatabase and
 *                    CO301_Interface). Empty if the function fails to find OD_name.
 **/
ObjectsDictionaryPtr getObjectDictionary(const char* OD_name);

/**
 * @ingroup can_open
 * Same as createObjectDictionary, but reload th file if it was already created.
 * Important: this operation can't be performed safely if any CO301_Interface instance is using the dictionary.
 * You should also invoke CO301_interface::rebuildObjectDatabase.
 *
 * @param OD_name     Identifier of the OD.
 * @param OD_name    This name will be used to identify in the future the object dictionary. It is used for example
 *                   by GetInstanceObjectDictionary and create_CO301_Interface.
 * @return            The smart pointer of the ObjectDictionary (to be used for example in the constructors of ObjectDatabase and
 *                    CO301_Interface). It doesn't change if the function fails to find OD_name.
 **/
ObjectsDictionaryPtr reloadObjectDictionary(const char* filename, const char* OD_name);


template <class T> T extractNumber(const char* &s );




class Minimal_CANopen_Dictionary: public ObjectsDictionary
{
public:
    Minimal_CANopen_Dictionary();
}; //end of class



} //end namespace


#endif // OBJECTDICTIONARY_H
