//
// You received this file as part of Finroc
// A framework for intelligent robot control
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    plugins/network_transport/runtime_info/tRemoteType.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-26
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tRemoteType.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/definitions.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace network_transport
{
namespace runtime_info
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

namespace
{

// Type name generation copied/adapter from rrlib/rtti/tType.cpp

template <typename T>
inline void StreamChars(std::ostream& stream, const T chars)
{
  stream << chars;
}

template <typename TStream>
void StreamType(TStream& stream, const tRemoteType& type)
{
  switch (type.GetTypeClassification())
  {
  case rrlib::rtti::tTypeClassification::RPC_TYPE:
  case rrlib::rtti::tTypeClassification::OTHER_DATA_TYPE:
  case rrlib::rtti::tTypeClassification::INTEGRAL:
  case rrlib::rtti::tTypeClassification::NULL_TYPE:
    StreamChars(stream, type.GetName());
    break;
  case rrlib::rtti::tTypeClassification::ARRAY:
    StreamChars(stream, "Array<");
    StreamType(stream, type.GetElementType());
    StreamChars(stream, ", ");
    char buffer[100];
    sprintf(buffer, "%d", type.GetArraySize());
    StreamChars(stream, buffer);
    StreamChars(stream, '>');
    break;
  case rrlib::rtti::tTypeClassification::LIST:
    StreamChars(stream, "List<");
    StreamType(stream, type.GetElementType());
    StreamChars(stream, '>');
    break;
  case rrlib::rtti::tTypeClassification::ENUM_BASED_FLAGS:
    StreamChars(stream, "EnumFlags<");
    StreamType(stream, type.GetElementType());
    StreamChars(stream, '>');
    break;
  case rrlib::rtti::tTypeClassification::PAIR:
    StreamChars(stream, "Pair<");
    StreamType(stream, type.GetTupleElementType(0));
    StreamChars(stream, ", ");
    StreamType(stream, type.GetTupleElementType(1));
    StreamChars(stream, '>');
    break;
  default:
    break;
  case rrlib::rtti::tTypeClassification::TUPLE:
    size_t tuple_size = type.GetTupleElementCount();
    StreamChars(stream, "Tuple<");
    for (size_t i = 0; i < tuple_size; i++)
    {
      StreamType(stream, type.GetTupleElementType(i));
      StreamChars(stream, i == tuple_size - 1 ? ">" : ", ");
    }
    break;
  }
}

rrlib::thread::tMutex mutables_mutex;
}

void tRemoteType::DeserializeRegisterEntry(rrlib::serialization::tInputStream& stream)
{
  *this = tRemoteType(); // Reset this object
  const bool legacy_procotol = stream.GetSourceInfo().revision == 0;
  type_register = rrlib::serialization::PublishedRegisters::GetRemoteRegister<tRemoteType>(stream);

  if (legacy_procotol)
  {
    enum { cLEGACY_ENUM_FLAG = 1 << 3 };

    stream.ReadShort(); // Default update time
    stream.ReadByte(); // Type classification

    name = stream.ReadString();
    types_checked = rrlib::rtti::tType::GetTypeCount();
    local_data_type = rrlib::rtti::tType::FindType(name);

    // read additional data we do not need in C++ (remote type traits and enum constant names)
    int8_t traits = stream.ReadByte(); // type traits
    if (traits & cLEGACY_ENUM_FLAG)
    {
      short n = stream.ReadShort();
      for (short i = 0; i < n; i++)
      {
        stream.SkipString();
      }
    }
  }
  else
  {
    type_traits = stream.ReadShort() << 8;
    underlying_type = (type_traits & rrlib::rtti::trait_flags::cHAS_UNDERLYING_TYPE) ? stream.ReadShort() : 0;
    switch (GetTypeClassification())
    {
    case rrlib::rtti::tTypeClassification::LIST:
    case rrlib::rtti::tTypeClassification::ENUM_BASED_FLAGS:
      element_type = stream.ReadShort();
      break;
    case rrlib::rtti::tTypeClassification::ARRAY:
      element_type = stream.ReadShort();
      array_size = stream.ReadInt();
      break;
    case rrlib::rtti::tTypeClassification::RPC_TYPE:
    case rrlib::rtti::tTypeClassification::INTEGRAL:
    case rrlib::rtti::tTypeClassification::OTHER_DATA_TYPE:
      name = stream.ReadString();
      types_checked = rrlib::rtti::tType::GetTypeCount();
      local_data_type = rrlib::rtti::tType::FindType(name);
      break;
    case rrlib::rtti::tTypeClassification::PAIR:
    case rrlib::rtti::tTypeClassification::TUPLE:
    {
      uint tuple_element_count = GetTypeClassification() == rrlib::rtti::tTypeClassification::TUPLE ? stream.ReadShort() : 2;
      tuple_elements.reserve(tuple_element_count);
      for (uint i = 0; i < tuple_element_count; i++)
      {
        tuple_elements.push_back(stream.ReadShort());
      }
    }
    break;
    case rrlib::rtti::tTypeClassification::NULL_TYPE:
      local_data_type = rrlib::rtti::tType();
      name = local_data_type.GetName();
      types_checked = rrlib::rtti::tType::GetTypeCount();
      break;
    default:
      throw std::runtime_error("Received erroneous type traits (invalid type classification)");
    }
  }
}

rrlib::rtti::tType tRemoteType::GetLocalDataType() const
{
  if (local_data_type)
  {
    return local_data_type;
  }
  rrlib::thread::tLock lock(mutables_mutex);
  if (local_data_type)
  {
    return local_data_type;
  }
  if (name.length() == 0)
  {
    GetName();
  }
  if (name.length() && types_checked < rrlib::rtti::tType::GetTypeCount())
  {
    types_checked = rrlib::rtti::tType::GetTypeCount();
    local_data_type = rrlib::rtti::tType::FindType(GetName());  // TODO: lookup by element types or tuple element types could also be done
  }
  return local_data_type;
}

const std::string& tRemoteType::GetName() const
{
  rrlib::thread::tLock lock(mutables_mutex);
  if (name.length() == 0)
  {
    std::stringstream stream;
    StreamType(stream, *this);
    name = stream.str();
  }
  return name;
}


void tRemoteType::SerializeRegisterEntry(rrlib::serialization::tOutputStream& stream, const rrlib::rtti::tType& type)
{
  const bool legacy_procotol = stream.GetTargetInfo().revision == 0;

  if (legacy_procotol)
  {
    stream.WriteShort(type.GetHandle());
    stream.WriteShort(-1); // Default update time (legacy, unused in C++)
    stream.WriteByte(0); // Type classification (legacy, unused in C++)
    stream.WriteString(type.GetName());
    stream.WriteByte(0); // type traits (legacy, unused in C++)
  }
  else
  {
    stream.WriteShort(static_cast<uint16_t>((type.GetTypeTraits() >> 8) & 0xFFFF));
    if (type.GetTypeTraits() & rrlib::rtti::trait_flags::cHAS_UNDERLYING_TYPE)
    {
      stream.WriteShort(type.GetUnderlyingType().GetHandle());
    }
    const bool java_client = stream.GetTargetInfo().custom_info & tSerializationInfoFlags::cJAVA_CLIENT;

    switch (type.GetTypeClassification())
    {
    case rrlib::rtti::tTypeClassification::LIST:
    case rrlib::rtti::tTypeClassification::ARRAY:
    case rrlib::rtti::tTypeClassification::ENUM_BASED_FLAGS:
      stream.WriteShort(type.GetElementType().GetHandle());
      if (type.IsArray())
      {
        stream.WriteInt(type.GetArraySize());
      }
      break;

    case rrlib::rtti::tTypeClassification::PAIR:
    case rrlib::rtti::tTypeClassification::TUPLE:
    {
      if (java_client)
      {
        stream.WriteInt(type.GetSize());
      }
      auto tuple_info = type.GetTupleTypes();
      if (type.GetTypeClassification() != rrlib::rtti::tTypeClassification::PAIR)
      {
        stream.WriteShort(tuple_info.second);
      }
      for (uint i = 0; i < tuple_info.second; i++)
      {
        stream.WriteShort(rrlib::rtti::tType(tuple_info.first[i].type_info).GetHandle());
      }
    }
    break;

    case rrlib::rtti::tTypeClassification::RPC_TYPE:
      stream.WriteString(type.GetPlainTypeName());
      break;

    case rrlib::rtti::tTypeClassification::INTEGRAL:
    case rrlib::rtti::tTypeClassification::OTHER_DATA_TYPE:
      stream.WriteString(type.GetPlainTypeName());
      if (java_client)
      {
        stream.WriteInt(type.GetSize());
        if (type.GetTypeTraits() & rrlib::rtti::trait_flags::cIS_ENUM)
        {
          const make_builder::internal::tEnumStrings* enum_strings = type.GetEnumStringsData();
          assert(enum_strings->size <= std::numeric_limits<short>::max() + 1); // more values would be quite ridiculous
          stream.WriteShort(static_cast<uint16_t>(enum_strings->size));
          bool send_values = enum_strings->non_standard_values;
          rrlib::rtti::tType underlying_type = type.GetUnderlyingType();
          const char* value_pointer = static_cast<const char*>(enum_strings->non_standard_values);
          stream.WriteByte(send_values ? underlying_type.GetSize() : 0); // Note that send_values is no type trait (flag) as it not a compile-time constant (at least not straight-forward)
          for (size_t j = 0; j < enum_strings->size; j++)
          {
            const char* enum_string = enum_strings->strings[static_cast<size_t>(make_builder::tEnumStringsFormat::NATURAL)][j];
            stream.WriteString(enum_string);
            if (send_values)
            {
              rrlib::rtti::tTypedConstPointer value(value_pointer, underlying_type);
              value.Serialize(stream);
              value_pointer += underlying_type.GetSize();
            }
          }
        }
      }
      break;
    case rrlib::rtti::tTypeClassification::NULL_TYPE:
      break;
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
