//
// You received this file as part of Finroc
// A framework for intelligent robot control
//
// Copyright (C) AG Robotersysteme TU Kaiserslautern
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
/*!\file    plugins/network_transport/runtime_info/tRemoteRuntime.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2014-09-19
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tRemoteRuntime.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tFrameworkElementTags.h"

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

tRemoteRuntime::tRemoteRuntime(const std::string& protocol, tFrameworkElement* parent, const tString& name, tFlags flags) :
  core::tFrameworkElement(parent, name, flags)
{
  core::tFrameworkElementTags::AddTag(*this, "remote_runtime: " + protocol);
}

void tRemoteRuntime::InitRemoteStructure(const rrlib::serialization::tFixedBuffer& current_structure_info)
{
  rrlib::serialization::tMemoryBuffer buffer;
  rrlib::serialization::tOutputStream stream(buffer);
  stream.Write(current_structure_info);
  stream.Close();
  structure_updates_port = data_ports::tOutputPort<rrlib::serialization::tMemoryBuffer>(this, "Structure", buffer);
  structure_updates_port.Init();
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
