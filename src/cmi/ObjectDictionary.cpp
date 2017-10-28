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

#include <fstream>
#include <vector>
#include "cmi/ObjectDictionary.h"
#include "cmi/globals.h"
#include "absl/strings/str_replace.h"

#define PRINTF if(0) printf

namespace CanMoveIt
{

inline void checkStreamString( std::stringstream& ss, const std::string& s)
{
    if( ss.fail() )
    {
        throw std::runtime_error ("Not a valid number ");
    }
    else while( !ss.eof() )
    {
        char c = '\r';
        ss >> c;
        if( c !='\r' && c !=' ' && c !='\n' )
        {
            throw std::runtime_error ("Not a valid number ");
        }
    }
}

template <typename T> T NumberParserHex(const std::string& s)
{
    uint64_t from;

    std::stringstream ss;
    ss << std::hex << s;
    ss >> from;
    checkStreamString(ss,s);

    if( sizeof(T) == 1 && from > 0xFF)
    {
        throw RangeException("Value too large.");
    }
    else if( sizeof(T) == 2 && from > 0xFFFF)
    {
        throw RangeException("Value too large.");
    }
    else if( sizeof(T) == 4 && from > 0xFFFFFFFF)
    {
        throw RangeException("Value too large.");
    }
    return static_cast<T>(from);
}

template <typename T> T NumberParser(const std::string& s)
{
    size_t pos = s.find("0x");
    if( pos == std::string::npos)
    {
        s.find("0X");
    }
    if( pos != std::string::npos)
    {
        std::string tmp = s;
        tmp.erase(0, pos+2);
        return NumberParserHex<T>(tmp);
    }
    //-------------------
    T to;
    std::stringstream ss;
    ss << s;

    if( std::is_integral<T>::value == false )
    {
        float from;
        ss >> from;
        checkStreamString(ss,s);
        Variant var( from );
        to = var.convert<T>();
    }
    else
    {
        int32_t from;
        ss >> from;
        checkStreamString(ss,s);
        Variant var( from );
        to = var.convert<T>();
    }

    return to;
}

inline bool compareObjectEntries( ObjectEntry a, ObjectEntry b) { return a.id().get() < b.id().get(); }

inline TypeID convert_EDS_Types( int const&  type )
{
    switch (type)
    {
        case 0x02:  return INT8;
        case 0x03:  return INT16;
        case 0x04:  return INT32;
        case 0x05:  return UINT8;
        case 0x06:  return UINT16;
        case 0x07:  return UINT32;
        case 0x08:  return FLOAT32;
        case 0x09:  return STRING;
        case 0x0F:  return STRING;
        case 0x1B:  return UINT64;
        default: return OTHER;
    }
    return OTHER;
}

inline Variant variantFromString(TypeID entry_type, const  std::string& line)
{
    switch (entry_type)
    {
        case INT8:    return Variant( NumberParser<int8_t>(line));
        case INT16:   return Variant( NumberParser<int16_t>(line));
        case INT32:   return Variant( NumberParser<int32_t>(line));

        case UINT8:   return Variant( NumberParser<uint8_t>(line));
        case UINT16:  return Variant( NumberParser<uint16_t>(line));
        case UINT32:  return Variant( NumberParser<uint32_t>(line));

        case FLOAT32:  return Variant( NumberParser<float>(line));

        default: return Variant();
    }
    return Variant();
}


class ObjectEntry::Impl{

public:
#ifndef EMBEDDED
    std::string parameter_name;
    Variant  default_value;
#endif
    ObjectID id;

    struct{
        uint8_t  access_type    : 2;
        uint8_t  PDO_mapable    : 1;
        uint8_t  raw_type       : 5;
    };

    Impl(): id(ObjectID(0,0)), raw_type(OTHER) {}
};

TypeID      ObjectEntry::type()            const   { return  static_cast<TypeID>(_d->raw_type ); }
uint8_t     ObjectEntry::size()            const   { return  getSize( type() );   }
ObjectID    ObjectEntry::id()              const   { return _d->id; }
uint16_t    ObjectEntry::index()           const   { return _d->id.index() ; }
uint8_t     ObjectEntry::subindex()        const   { return _d->id.subindex() ;  }
bool        ObjectEntry::PDO_is_mapable()  const   { return _d->PDO_mapable ; }
ObjectEntry::AccessType  ObjectEntry::access_type()     const   { return static_cast<AccessType>(_d->access_type); }
bool        ObjectEntry::empty()           const   { return ( type() == OTHER || _d->id == ObjectID(0,0)); }
const char* ObjectEntry::name()            const   { return _d->parameter_name.c_str(); }

Variant  ObjectEntry::default_value()   const   { return _d->default_value; }

ObjectEntry::ObjectEntry(): _d( new Impl)
{

}

ObjectEntry::ObjectEntry(uint16_t index,
                         uint8_t subindex,
                         AccessType access,
                         bool mappable,
                         TypeID type,
                         const char* description,
                         Variant default_val)
    :_d( new Impl)
{
    _d->id = ObjectID(index, subindex );
    _d->access_type = (access);
    _d->PDO_mapable = (mappable);
    _d->raw_type = type;
#ifndef EMBEDDED
    if(description) _d->parameter_name = std::string(description);
    _d->default_value = default_val;
#endif
}


void ObjectEntry::print() const
{
#ifndef EMBEDDED
    printf("Entry with name: %s\n", _d->parameter_name.c_str());
#endif
    printf("index: 0x%X    subindex: 0x%X\n", _d->id.index(), _d->id.subindex());
    printf("access type = %d\n", _d->access_type);
    printf("type = %s\n", toStr( type() ) );
}
//--------------------------------------------------------------
struct ObjectsDictionary::Impl{
    std::vector<ObjectEntry>  object_entries;
    ObjectEntry parseBlock(std::stringstream & ss, int num_lines );
    std::map<uint16_t,std::string> array_index_name;
     uint32_t vendor;
     uint32_t product;
     uint32_t revision ;
};



ObjectsDictionary::ObjectsDictionary() : _d(new Impl) {}
ObjectsDictionary::~ObjectsDictionary() { delete _d;}

void ObjectsDictionary::insert(ObjectEntry const& obj)
{
    _d->object_entries.push_back( obj );
}


void ObjectsDictionary::clear()
{
    _d->object_entries.clear();
}

uint16_t ObjectsDictionary::size() const { return _d->object_entries.size(); }


ObjectEntry& ObjectsDictionary::at( uint16_t position) { return _d->object_entries.at(position); }

//------------------------------------------------------------------

void indentMore(std::string *ind)
{
    ind->append("    ");
}

void indentLess(std::string *ind)
{
    if( ind->size() >=4) ind->erase(0,4);
}


uint32_t ObjectsDictionary::vendorNumber() const   { return _d->vendor; }
uint32_t ObjectsDictionary::productNumber() const  { return _d->product; }
uint32_t ObjectsDictionary::revisionNumber() const { return _d->revision; }


void ObjectsDictionary::generateCode(const char* name)
{
    std::ofstream  fout;
    std::string ind;
    std::string class_name = name + std::string("_Dictionary");
    //----------------------------

    std::string header_filename = name + std::string(".h");
    fout.open (header_filename.c_str());

    fout << ind << "#ifndef "<< name << "_DICTIONARY_H_\n";
    fout << ind << "#define "<< name << "_DICTIONARY_H_\n";
    fout << ind << "#include \"cmi/ObjectDictionary.h\" " << "\n\n";
    fout << ind << "namespace CanMoveIt {"  << "\n\n";
    indentMore(&ind);
    fout << ind << "class "<< class_name << ": public ObjectsDictionary\n";

    fout << ind << "{ \n";
    fout << ind << "public:\n";
    indentMore(&ind);

    fout << ind << class_name << "(); \n\n";

    indentLess(&ind);
    fout << ind << "}; //end of class\n";
    indentLess(&ind);
    fout << ind << "} //end of namespace\n\n";
    fout << ind << "#endif \n";
    fout.close();

    //----------------------------
    std::string source_filename = name + std::string(".cpp");
    fout.open (source_filename.c_str());

    fout << "// This file was generated by ObjectsDictionary::generateCode \n\n";
    fout << ind << "#include \"" << header_filename << "\" \n\n";
    fout << ind << "namespace CanMoveIt {"  << "\n\n";

    indentMore(&ind);
    fout << ind << class_name<<"::" << class_name << "() \n";
    fout << ind << "{\n";
    indentMore(&ind);

    fout << ind << "this->_d->object_entries.reserve("<< _d->object_entries.size() << " );\n";

    for(unsigned i=0; i< _d->object_entries.size(); i++)
    {
        ObjectEntry *entry = &_d->object_entries.at(i);
        if( entry->subindex() == 0 )  fout << "//index 0x" << std::hex << entry->index() << std::dec <<  "\n";

        fout << ind << "this->insert( ObjectEntry(";
        printf("generating index 0x%X subindex 0x%X\n", entry->index(), entry->subindex() );

        fout << "0x" << std::hex << entry->index() << std::dec <<  ",\t";
        fout << std::dec << (int)entry->subindex()     << ",\t\t";

        if (entry->access_type() == ObjectEntry::RO)    fout << "ObjectEntry::RO";
        if (entry->access_type() == ObjectEntry::RW)    fout << "ObjectEntry::RW";
        if (entry->access_type() == ObjectEntry::WO)    fout << "ObjectEntry::WO";
        if (entry->access_type() == ObjectEntry::CNST)  fout << "ObjectEntry::CNST";
        fout << ",\t";

        fout << entry->PDO_is_mapable() << ",\t";
        fout << toStr( entry->type() )  << " ) );\n";
    }

    indentLess(&ind);
    fout << ind << "\n";
    fout << ind << "} //end of constructor\n";
    indentLess(&ind);
    fout << ind << "} //end of namespace\n\n";

    fout.close();
    //--------------------------------------------------
}


ObjectsDictionaryPtr createObjectDictionary(const char* filename, const char* OD_name)
{

    if (CMI::get().object_dictionaries.find(OD_name) != CMI::get().object_dictionaries.end())
    {
        printf("You already allocated an ObjectDictionary with the same identifier [%s]\n",OD_name );
        return CMI::get().object_dictionaries.at(OD_name);
    }

    // create a file-reading object
    std::ifstream fin;
    fin.open(filename); // open a file
    if (!fin.good())
    {
        printf("problem loading %s \n", filename);
        throw std::runtime_error("Problem loading EDS file" );
    }

    ObjectsDictionaryPtr od_ptr( new ObjectsDictionary );
    od_ptr->parseEDS( fin );
    CMI::get().object_dictionaries.insert( std::make_pair(OD_name, od_ptr ) );

    return od_ptr;
}

ObjectsDictionaryPtr reloadObjectDictionary(const char* filename, const char* OD_name)
{
    std::ifstream fin;
    fin.open(filename); // open a file
    if (!fin.good())
    {
        printf("problem loading %s \n", filename);
        throw std::runtime_error("Problem loading EDS file" );
    }

    ObjectsDictionaryPtr od_ptr;
    if (CMI::get().object_dictionaries.find(OD_name) == CMI::get().object_dictionaries.end())
    {
        // not found. Allocate one
        od_ptr = ObjectsDictionaryPtr( new ObjectsDictionary );
        od_ptr->parseEDS( fin );
        CMI::get().object_dictionaries.insert( std::make_pair(OD_name, od_ptr ) );
    }
    else{
        od_ptr = CMI::get().object_dictionaries.at(OD_name);
        od_ptr->parseEDS( fin );
    }
    return od_ptr;
}


ObjectsDictionaryPtr getObjectDictionary(const char* OD_name)
{
    if (CMI::get().object_dictionaries.find(OD_name) == CMI::get().object_dictionaries.end() )
    {
        if( strcmp(OD_name, "Minimal_CANopen_Dictionary")== 0  )
        {
            ObjectsDictionaryPtr minimal_dictionary( new Minimal_CANopen_Dictionary );
            CMI::get().object_dictionaries.insert(std::make_pair(OD_name,minimal_dictionary));
            return minimal_dictionary;
        }
        return ObjectsDictionaryPtr(); // empty
    }
    return  CMI::get().object_dictionaries.at(OD_name);
}

const ObjectEntry& ObjectsDictionary::getEntry(ObjectKey const& key)
{
    return _d->object_entries.at(key);
}


ObjectKey ObjectsDictionary::find(uint16_t index, uint8_t subindex)
{
    ObjectEntry entry_to_find( index,  subindex, ObjectEntry::RO, false, OTHER);
    std::vector<ObjectEntry>::iterator it;

    it = std::lower_bound (_d->object_entries.begin(), _d->object_entries.end(), entry_to_find, compareObjectEntries);

    bool found = true;
    if( it == _d->object_entries.end() )
    {
        // printf("this is not there:  0x%X\n", entry_to_find.id().get() );
        found = false;
    }
    else if( it->id().get() !=  entry_to_find.id().get() )
    {
       // printf("DONT match: 0x%X  0x%X\n", it->id().get(), entry_to_find.id().get());
        found = false;
    }
    if( !found)
    {
        char temp[100];
        sprintf(temp,"index/subindex pair 0x%X/0x%x not found in object dictionary\n", index, subindex);
     //   printf("%s\n", temp);

        throw std::runtime_error( temp );
    }
    return  ObjectKey( it - _d->object_entries.begin() );
}

std::string GetLine(std::ifstream & fin)
{
    char buffer[100];
    fin.getline(buffer, 100);
    return std::string(buffer);
}

std::string GetLine(std::stringstream & fin)
{
    char buffer[100];
    fin.getline(buffer, 100);
    return std::string(buffer);
}

bool lineIsObjectHeader(std::string const& line)
{
    return ( line.find('[') != std::string::npos && line.find(']') != std::string::npos);
}

void ObjectsDictionary::parseEDS(std::ifstream& fin)
{
    // clean up just in case.
    _d->object_entries.clear();
    _d->array_index_name.clear();

    std::stringstream block;
    std::string line =GetLine(fin);

    while (lineIsObjectHeader(line) == false && !fin.eof()) line =GetLine(fin);

    while( !fin.eof() )
    {
        int num_lines = 0;
        block.str(line);
        do{
            //clear the block
            block << line << "\n";
            num_lines++;
            line =GetLine(fin);
        }
        while(lineIsObjectHeader(line) == false && !fin.eof() );

        ObjectEntry new_entry = _d->parseBlock( block , num_lines) ;

        if(new_entry.type() != OTHER && new_entry.type() != ARRAY_INDEX )
        {
            _d->object_entries.push_back( new_entry );
            //  printf("Added [0x%X / 0x%X]: \n", _d->object_entries.back().index(), _d->object_entries.back().subindex());
        }
    }

    // IMPORTANT sort at the end
    std::sort( _d->object_entries.begin(), _d->object_entries.end(), compareObjectEntries );
}

void RemoveSpaces(std::string &s)
{
    size_t first;

    //-------- remove certain elements -------
    const std::string characters_to_remove(".:;/\\ -\"\'");
    first = s.find_first_of(characters_to_remove);
    while( first != std::string::npos)
    {
        s.at(first) = '_';
        first = s.find_first_of(characters_to_remove);
    }

    //---- remove doubles  -------
    first =  s.find("__");
    while( first != std::string::npos)
    {
        s.erase(first,1);
        first = s.find("__");
    }

    first =  s.find_first_of("\r\0");
    while( first != std::string::npos)
    {
        s.erase(first,1);
        first = s.find_first_of("\r\0");
    }

    while (s.at( s.size()-1 )=='_') s.erase(s.size()-1,1);
}

void RemoveParameterName(std::string &s)
{
    size_t pos = s.find('=');
    if( pos == 0)
    {
        throw std::runtime_error( std::string("no parameter name in EDS ") + s );
    }
    s.erase(0, pos+1);
}

ObjectEntry ObjectsDictionary::Impl::parseBlock(std::stringstream & ss, int num_lines )
{
    uint16_t    entry_index    = 0;
    uint8_t     entry_subindex = 0;
    bool        entry_mappable = false;
    TypeID      entry_type( OTHER);
    ObjectEntry::AccessType  entry_access = ObjectEntry::CNST;
    std::string entry_parameter_name;
    Variant  default_val;

    PRINTF("\n------------------------\n");

    //first line is the header for sure
    std::string line = GetLine(ss);

    size_t first_indx,last, first_sub;
    bool IS_SUB = false;
    bool IS_ARRAY_INDEX = false;

    first_indx =  line.find('[')+1;
    last       =  line.find(']')-1;
    first_sub  = line.find("sub");

    //-------------------------------------------------
    // Parse the [DeviceInfo]
    if( line.compare(0,12,"[DeviceInfo]") == 0)
    {

        for (int i=0; i< num_lines-1; i++)
        {
            line = GetLine(ss);
            if( line.find("VendorNumber=") != std::string::npos )
            {
                RemoveParameterName(line);
                this->vendor = Variant(line).convert<uint32_t>();
            }
            if( line.find("ProductNumber=") != std::string::npos )
            {
                RemoveParameterName(line);
                this->product = Variant(line).convert<uint32_t>();
            }
            if( line.find("RevisionNumber=") != std::string::npos )
            {
                RemoveParameterName(line);
                this->revision = Variant(line).convert<uint32_t>();
            }
        }
        return ObjectEntry();
    }
    //-------------------------------------------------
    // Parse a block that contain index (and optionally subindex)

    bool skip_block = false;
    //case 1: index only

    int obj_index =-1;
    if ( first_sub !=  std::string::npos )
    {
        if ( (first_sub - first_indx) != 4 ) skip_block = true;
    }
    else if ( (last - first_indx) >4)
    {
        skip_block = true;
    }

    if( skip_block == false)
    {

        obj_index =  NumberParserHex<unsigned>(line.substr(first_indx, first_indx+3)) ;
    }

    if(obj_index <= 0 || skip_block)
    {
        PRINTF("BLOCK skipped. Header: %s\n",line.c_str());
        return ObjectEntry();
    }
    //std::cout << line << std::endl;
    PRINTF("-----------\nindex: 0x%X ( %d ) ", obj_index, obj_index);

    int subindex = 0;
    if ( first_sub != std::string::npos)
    {
        IS_SUB = true;
        std::string temp_s = line.substr( first_sub+3, last - first_sub-2 );
        subindex = NumberParserHex<unsigned>( temp_s );
        PRINTF("subindex: %d", subindex);
    }
    PRINTF("\n");

    entry_index    = obj_index;
    entry_subindex = subindex;
    //---------------------------------------------------

    for (int i=0; i< num_lines-1; i++)
    {
        line = GetLine(ss);
        //   std::cout << "parsing line: " << line << std::endl;

        if( line.find("SubNumber=") != std::string::npos &&  IS_SUB == false)
        {
            RemoveParameterName(line);
            int sub_index_count = Variant(line).convert<int>();

            PRINTF(" SubNumber =  %d\n",  sub_index_count);

            if( sub_index_count != 0 )
            {
                IS_ARRAY_INDEX = true;
            }
        }
        //-------------------------------------------------

        if( line.find("ParameterName=") != std::string::npos)
        {
#ifndef EMBEDDED
            RemoveParameterName(line);
            RemoveSpaces(line);

            if( IS_ARRAY_INDEX )
            {
                array_index_name.insert( std::make_pair(entry_index, line ) );
            }
            else if(IS_SUB)
            {
                entry_parameter_name = array_index_name[entry_index];
                entry_parameter_name.append("::").append(line );
            }
            else{
                entry_parameter_name = line;
            }
            PRINTF(" ParameterName = %s\n",entry_parameter_name.c_str());
#endif
        }

        //-------------------------------------------------

        if( line.find("ObjectType=") != std::string::npos)
        {
            RemoveParameterName(line );

            PRINTF(" ObjectType =  %s\n",  line.c_str() );
        }

        //-------------------------------------------------
        if( IS_ARRAY_INDEX == false)
        {
            if( line.find("AccessType=") != std::string::npos)
            {
                RemoveParameterName(line );

                std::locale loc;
                for (char& c: line){
                    c = std::toupper( c, loc);
                }

                if( line.find("RO") != std::string::npos)     entry_access = ObjectEntry::RO;
                if( line.find("WO") != std::string::npos)     entry_access = ObjectEntry::WO;
                if( line.find("RW") != std::string::npos)     entry_access = ObjectEntry::RW;
                if( line.find("CONST") != std::string::npos)  entry_access = ObjectEntry::CNST;
                PRINTF(" AccessType =  %d\n",  entry_access);

            }
            //-------------------------------------------------

            if( line.find("DataType=") != std::string::npos)
            {
                std::string prev = line;
                RemoveParameterName(line );
                int raw_type =  Variant(line).convert<int>() ;

                // see http://ms10.at.tut.by/eds.html
                entry_type = convert_EDS_Types(raw_type);

                PRINTF(" DataType =  0x%X (%s)\n",
                       raw_type, toStr(entry_type));

                if (entry_type == OTHER)
                {
                    // not recognized: skip it
                    PRINTF(" AccessType =  not recognized\n");
                    return ObjectEntry();
                }
            }
            //-------------------------------------------------

            if( line.find("DefaultValue=") != std::string::npos)
            {

#ifndef EMBEDDED
                RemoveParameterName(line );
                if( line.size() > 0)
                {
                    default_val = variantFromString(entry_type, line);
                }
#endif
                /*printf(" DefaultValue =  %s  -> %s -> %d  -> 0x%X\n",  line.c_str() , default_val.convert<std::string>().c_str(),
                        default_val.convert<int>(), default_val.convert<int>() );*/
            }
            //-------------------------------------------------
            if( line.find("PDOMapping=") != std::string::npos)
            {
                RemoveParameterName(line );
                entry_mappable= Variant(line).convert<bool>();
                PRINTF(" PDOMapping =  %d (line was %s)\n", entry_mappable, line.c_str() );
            }
        }
    }

    if( IS_ARRAY_INDEX == true )
    {
#ifdef EMBEDDED
        return ObjectEntry();
#else
        return ObjectEntry(entry_index, 0, ObjectEntry::CNST, 0, ARRAY_INDEX, entry_parameter_name.c_str());
#endif
    }
    return ObjectEntry(entry_index, entry_subindex,
                       entry_access, entry_mappable, entry_type,
                       entry_parameter_name.c_str(),
                       default_val);
}

Minimal_CANopen_Dictionary::Minimal_CANopen_Dictionary()
{
//index 0x1000
    this->insert( ObjectEntry( 0x1000,	0,		ObjectEntry::RO,	0,	UINT32, "CAN_Controller_Type" ) );
//index 0x1001
    this->insert( ObjectEntry( 0x1001,	0,		ObjectEntry::RO,	0,	UINT8 , "Error_Register") );
//index 0x1018
    this->insert( ObjectEntry( 0x1018,	0,		ObjectEntry::CNST,	0,	UINT8 , "Identity_Object::Number_of_Entries") );
    this->insert( ObjectEntry( 0x1018,	1,		ObjectEntry::RO,	0,	UINT32, "Identity_Object::Vendor_ID" ) );
    this->insert( ObjectEntry( 0x1018,	2,		ObjectEntry::RO,	0,	UINT32, "Identity_Object::Product_Code" ) );
    this->insert( ObjectEntry( 0x1018,	3,		ObjectEntry::RO,	0,	UINT32, "Identity_Object::Revision_Number" ) );
    this->insert( ObjectEntry( 0x1018,	4,		ObjectEntry::RO,	0,	UINT32, "Identity_Object::Serial_Number" ) );
}

} //end namespace

