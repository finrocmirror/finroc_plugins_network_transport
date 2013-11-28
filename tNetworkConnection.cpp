//
// You received this file as part of Finroc
// A Framework for intelligent robot control
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
/*!\file    plugins/network_transport/tNetworkConnection.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/tNetworkConnection.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

tNetworkConnection::tNetworkConnection() :
  encoding(tDestinationEncoding::NONE),
  uuid(),
  port_handle(0)
{}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnection& connection)
{
  stream << connection.encoding;
  switch (connection.encoding)
  {
  case tDestinationEncoding::NONE:
    break;
  case tDestinationEncoding::UUID_AND_HANDLE:
    stream << connection.uuid << connection.port_handle;
    break;
  default:
    FINROC_LOG_PRINT(ERROR, "Unsupported encoding");
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnection& connection)
{
  stream >> connection.encoding;
  switch (connection.encoding)
  {
  case tDestinationEncoding::NONE:
    break;
  case tDestinationEncoding::UUID_AND_HANDLE:
    stream >> connection.uuid >> connection.port_handle;
    break;
  default:
    FINROC_LOG_PRINT(ERROR, "Unsupported encoding");
  }
  return stream;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
