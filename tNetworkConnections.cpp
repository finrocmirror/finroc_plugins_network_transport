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
/*!\file    plugins/network_transport/tNetworkConnections.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/tNetworkConnections.h"

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

//----------------------------------------------------------------------
// tNetworkConnections constructors
//----------------------------------------------------------------------
tNetworkConnections::tNetworkConnections() :
  connections()
{}


rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnections& connections)
{
  stream.WriteInt(static_cast<uint>(connections.connections.size()));
  for (size_t i = 0; i < connections.connections.size(); i++)
  {
    stream << connections.connections[i];
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnections& connections)
{
  connections.connections.clear();
  int count = stream.ReadInt();
  for (int i = 0; i < count; i++)
  {
    connections.connections.emplace_back();
    stream >> connections.connections.back();
  }
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
