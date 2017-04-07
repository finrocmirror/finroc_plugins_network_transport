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
/*!\file    plugins/network_transport/generic_protocol/tServerSideConversionInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-18
 *
 * \brief   Contains tServerSideConversionInfo
 *
 * \b tServerSideConversionInfo
 *
 * Information on server-side data conversion in network connectors
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tServerSideConversionInfo_h__
#define __plugins__network_transport__generic_protocol__tServerSideConversionInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace network_transport
{
namespace generic_protocol
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Server-side conversion info
/*!
 * Information on server-side data conversion in network connectors
 */
struct tServerSideConversionInfo
{

  /*! Parameters on server-side data conversion */
  std::string operation_1, operation_2, operation_1_parameter, operation_2_parameter, intermediate_type, destination_type;

  bool NoConversion() const
  {
    return operation_1.length() == 0 && operation_2.length() == 0 && operation_1_parameter.length() == 0 && operation_2_parameter.length() == 0 && intermediate_type.length() == 0 && destination_type.length() == 0;
  }
};


inline bool operator==(const tServerSideConversionInfo& lhs, const tServerSideConversionInfo& rhs)
{
  return lhs.operation_1 == rhs.operation_1 && lhs.operation_2 == rhs.operation_2 && lhs.operation_1_parameter == rhs.operation_1_parameter &&
         lhs.operation_2_parameter == rhs.operation_2_parameter && lhs.intermediate_type == rhs.intermediate_type && lhs.destination_type == rhs.destination_type;
}

inline bool operator!=(const tServerSideConversionInfo& lhs, const tServerSideConversionInfo& rhs)
{
  return !(lhs == rhs);
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tServerSideConversionInfo& info)
{
  stream << info.operation_1 << info.operation_1_parameter << info.operation_2 << info.operation_2_parameter << info.intermediate_type << info.destination_type;
  return stream;
}

inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tServerSideConversionInfo& info)
{
  stream >> info.operation_1 >> info.operation_1_parameter >> info.operation_2 >> info.operation_2_parameter >> info.intermediate_type >> info.destination_type;
  return stream;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
