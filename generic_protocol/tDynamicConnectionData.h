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
/*!\file    plugins/network_transport/generic_protocol/tDynamicConnectionData.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-13
 *
 * \brief   Contains tDynamicConnectionData
 *
 * \b tDynamicConnectionData
 *
 * Dynamic data on port connections (subscriptions) over the network
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tDynamicConnectionData_h__
#define __plugins__network_transport__generic_protocol__tDynamicConnectionData_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tDynamicConnectorParameters.h"

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
//! Dynamic connection data
/*!
 * Dynamic data on port connections (subscriptions) over the network
 */
struct tDynamicConnectionData : public tDynamicConnectorParameters
{
  /*! Strategy to use (-1 no subscription, 0 pull subscription, 1+ push subscription with specified queue length) */
  uint16_t strategy = -1;

};

inline bool operator==(const tDynamicConnectionData& lhs, const tDynamicConnectionData& rhs)
{
  return static_cast<const tDynamicConnectorParameters&>(lhs) == static_cast<const tDynamicConnectorParameters&>(rhs) && lhs.strategy == rhs.strategy;
}
inline bool operator!=(const tDynamicConnectionData& lhs, const tDynamicConnectionData& rhs)
{
  return !(lhs == rhs);
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tDynamicConnectionData& data)
{
  stream << static_cast<const tDynamicConnectorParameters&>(data) << data.strategy;
  return stream;
}

inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tDynamicConnectionData& data)
{
  stream >> static_cast<tDynamicConnectorParameters&>(data) >> data.strategy;
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
