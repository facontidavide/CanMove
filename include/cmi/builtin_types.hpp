
/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright 2016-2017 Davide Faconti
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
* *******************************************************************/

#ifndef ROS_BUILTIN_TYPES_HPP
#define ROS_BUILTIN_TYPES_HPP

#include <stdint.h>
#include <string>
#include <iostream>
#include <unordered_map>
#include "OS/os_abstraction.h"

namespace CanMoveIt{


// These types are consistent with CANOpen representation
enum TypeID {
    UINT8  = 0 | 0x8,
    UINT16 = 1 | 0x8,
    UINT32 = 2 | 0x8,
    UINT64 = 3 | 0x8,

    INT8   = 0 | 0xC,
    INT16  = 1 | 0xC,
    INT32  = 2 | 0xC,
    INT64 =  3 | 0xC,

    FLOAT32 = 0 | 0x00,
    FLOAT64 = 1 | 0x00,
    STRING  = 2 | 0x00,

    ARRAY_INDEX = 0 | 0x04,
    OTHER       = 1 | 0x04,
};

//---------------------------------------------------------

inline int builtinSize(const TypeID c) {
  switch (c) {
  case INT8:
  case UINT8:    return 1;
  case UINT16:
  case INT16:    return 2;
  case UINT32:
  case INT32:
  case FLOAT32:  return 4;
  case UINT64:
  case INT64:
  case FLOAT64:
  case STRING:
  case OTHER: return -1;
  }
  throw std::runtime_error("unsupported builtin type value");
}

inline const char* toStr(const TypeID& c)
{
  switch (c) {
  case INT8:     return "INT8";
  case UINT8:    return "UINT8";
  case UINT16:   return "UINT16";
  case UINT32:   return "UINT32";
  case UINT64:   return "UINT64";
  case INT16:    return "INT16";
  case INT32:    return "INT32";
  case INT64:    return "INT64";
  case FLOAT32:  return "FLOAT32";
  case FLOAT64:  return "FLOAT64";
  case STRING:   return "STRING";
  case OTHER:    return "OTHER";
  }
  throw std::runtime_error("unsupported builtin type value");
}

inline std::ostream& operator<<(std::ostream& os, const TypeID& c)
{
  os << toStr(c);
  return os;
}

template <typename T> TypeID getType()
{
    return OTHER;
}

inline bool isSigned(TypeID type)
{
    switch (type) {
    case UINT8:
    case UINT16:
    case UINT32:
    case UINT64:  return false;
    default:      return true;
    }
    return false;
}


inline int getSize(TypeID type) {

    switch( type )
    {

    case UINT8:
    case INT8:  return 1;

    case INT16:
    case UINT16: return 2;

    case INT32:
    case UINT32:
    case FLOAT32: return 4;

    case INT64:
    case UINT64:
    case FLOAT64: return 8;

    default: return -1;
    }
}

template <> inline TypeID getType<int8_t>()  {  return INT8; }
template <> inline TypeID getType<int16_t>() {  return INT16; }
template <> inline TypeID getType<int32_t>() {  return INT32; }
template <> inline TypeID getType<int64_t>() {  return INT64; }

template <> inline TypeID getType<uint8_t>()  {  return UINT8; }
template <> inline TypeID getType<uint16_t>() {  return UINT16; }
template <> inline TypeID getType<uint32_t>() {  return UINT32; }
template <> inline TypeID getType<uint64_t>() {  return UINT64; }

template <> inline TypeID getType<float>()  {  return FLOAT32; }
template <> inline TypeID getType<double>() {  return FLOAT64; }

template <> inline TypeID getType<std::string>() {  return STRING; }


}

#endif // ROS_BUILTIN_TYPES_HPP
