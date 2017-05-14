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
/*!\file    plugins/network_transport/generic_protocol/tUriConnector.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-06
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tUriConnector.h"

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
namespace generic_protocol
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
// tUriConnector constructors
//----------------------------------------------------------------------
tUriConnector::tUriConnector(core::tAbstractPort& owner_port, const rrlib::uri::tURI& uri, const rrlib::uri::tURIElements& parsed_uri, const core::tUriConnectOptions& connect_options, tNetworkTransportPlugin& plugin) :
  tBase(owner_port, uri, connect_options, plugin),
  parameter_array(
{
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.operation_1),
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.operation_2),
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.operation_1_parameter),
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.operation_2_parameter),
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.intermediate_type),
  rrlib::rtti::tTypedConstPointer(&static_parameters.server_side_conversion.destination_type),
  rrlib::rtti::tTypedConstPointer(&dynamic_parameters.minimal_update_interval),
  rrlib::rtti::tTypedConstPointer(&dynamic_parameters.high_priority)
}),
local_runtime_info(*plugin.LocalRuntimeInfo())
{
  this->flags |= core::tConnectionFlag::NAMED_PARAMETERS;
  assert(plugin.LocalRuntimeInfo());
  this->owner_port = &owner_port;
  uri_connector_address = this;
  static_parameters.server_port_id.path = parsed_uri.path;
  static_parameters.server_port_id.authority = parsed_uri.authority;

  SetParametersInConstructor(connect_options);

  PublishStructureChangeEvent(new tLocalRuntimeInfo::tUriConnectorAddEvent(*this, ConversionOperations()));
}

void tUriConnector::OnDisconnect()
{
  PublishStructureChangeEvent(new tLocalRuntimeInfo::tUriConnectorDeleteEvent(*this));
}

void tUriConnector::PublishStructureChangeEvent(tLocalRuntimeInfo::tStructureChangeEvent* event)
{
  std::shared_ptr<tLocalRuntimeInfo::tStructureChangeEvent> shared_event(event);
  std::unique_ptr<tLocalRuntimeInfo::tSharedStructureChangeEvent> to_enqueue(new tLocalRuntimeInfo::tSharedStructureChangeEvent(shared_event));
  local_runtime_info.incoming_structure_changes.Enqueue(std::move(to_enqueue));
}

bool tUriConnector::SetParameter(size_t index, const rrlib::rtti::tTypedConstPointer& new_value)
{
  bool changed = tBase::SetParameter(index, new_value);
  if (changed)
  {
    PublishStructureChangeEvent(new tLocalRuntimeInfo::tUriConnectorChangeEvent(*this));
  }
  return changed;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
