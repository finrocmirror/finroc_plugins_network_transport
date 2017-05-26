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
/*!\file    plugins/network_transport/generic_protocol/tNetworkPortInfo.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-02-23
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfo.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfoClient.h"

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

tNetworkPortInfo::tNetworkPortInfo(tRemoteRuntime& remote_runtime, tHandle connection_handle, tHandle remote_port_handle, int16_t strategy, core::tAbstractPort& port, tHandle served_port_handle) :
  connection_handle(connection_handle),
  remote_port_handle(remote_port_handle),
  strategy(strategy),
  remote_runtime(remote_runtime),
  values_to_send(),
  deleted(false),
  last_update(rrlib::time::cNO_TIME),
  desired_encoding(rrlib::serialization::tDataEncoding::BINARY),
  served_port_handle(served_port_handle)
{
  port.AddAnnotation(*this);
  ChangeStrategy(strategy);
}

void tNetworkPortInfo::ChangeStrategy(int16_t new_strategy)
{
  strategy = new_strategy;
  core::tAbstractPort* port = this->GetAnnotated<core::tAbstractPort>();
  if (data_ports::IsDataFlowType(port->GetDataType()))
  {
    if (port->IsOutputPort())
    {
      values_to_send.SetMaxLength(port->GetFlag(core::tFrameworkElement::tFlag::PUSH_STRATEGY_REVERSE) ? 1 : 0);
    }
    else
    {
      values_to_send.SetMaxLength(strategy < 0 ? 0 : strategy);
    }
  }
}

void tNetworkPortInfo::OnManagedDelete()
{
  if (client_info)
  {
    client_info->OnManagedDelete();
  }

  // TODO
  //assert((core::tRuntimeEnvironment::ShuttingDown() || rrlib::thread::tThread::CurrentThreadId() == remote_runtime.GetPeerImplementation().GetTCPThreadId()) && "Deleting should only be done by TCP thread");
  deleted = true;
  remote_runtime.PortDeleted(*this);
}

void tNetworkPortInfo::SetServerSideDynamicConnectionData(const tDynamicConnectionData& data)
{
  this->current_dynamic_connection_data = data;
  this->strategy = data.strategy;
  core::tAbstractPort* port = GetAnnotated<core::tAbstractPort>();
  if (data_ports::IsDataFlowType(port->GetDataType()))
  {
    if (!port->IsOutputPort())
    {
      values_to_send.SetMaxLength(strategy < 0 ? 0 : strategy);
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
