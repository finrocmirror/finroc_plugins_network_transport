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
/*!\file    plugins/network_transport/generic_protocol/tStaticConnectorParameters.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-08
 *
 * \brief   Contains tStaticConnectorParameters
 *
 * \b tStaticConnectorParameters
 *
 * Static parameters on network connectors
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tStaticConnectorParameters_h__
#define __plugins__network_transport__generic_protocol__tStaticConnectorParameters_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tServerSideConversionInfo.h"

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
//! Static network connector parameters
/*!
 * Static parameters on network connectors
 */
template <typename TServerPortId>
struct tStaticConnectorParameters
{

  /*! Parameters on server-side data conversion */
  tServerSideConversionInfo server_side_conversion;

  /*! ID of server port */
  TServerPortId server_port_id;

  /*! Whether to subscribe at input port for reverse pushing */
  bool reverse_push;

  tStaticConnectorParameters() :
    server_port_id(TServerPortId()),
    reverse_push(false)
  {}

  /*! Copy constructor replacing server port id */
  template <typename TOtherId>
  tStaticConnectorParameters(const tStaticConnectorParameters<TOtherId>& other, const TServerPortId& this_id) :
    server_side_conversion(other.server_side_conversion),
    server_port_id(this_id),
    reverse_push(other.reverse_push)
  {}

};


template <typename TServerPortId>
inline bool operator==(const tStaticConnectorParameters<TServerPortId>& lhs, const tStaticConnectorParameters<TServerPortId>& rhs)
{
  return lhs.server_port_id == rhs.server_port_id && lhs.reverse_push == rhs.reverse_push && lhs.server_side_conversion == rhs.server_side_conversion;
}

template <typename TServerPortId>
inline bool operator!=(const tStaticConnectorParameters<TServerPortId>& lhs, const tStaticConnectorParameters<TServerPortId>& rhs)
{
  return !(lhs == rhs);
}

template <typename TServerPortId>
inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tStaticConnectorParameters<TServerPortId>& data)
{
  stream << data.server_port_id << data.reverse_push << data.server_side_conversion;
  return stream;
}

template <typename TServerPortId>
inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tStaticConnectorParameters<TServerPortId>& data)
{
  stream >> data.server_port_id >> data.reverse_push >> data.server_side_conversion;
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
