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
/*!\file    plugins/network_transport/generic_protocol/tNetworkPortInfoClient.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-12
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfoClient.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkConnector.h"

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

tNetworkPortInfoClient::tNetworkPortInfoClient(tNetworkPortInfo& network_port_info, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info) :
  network_port_info(network_port_info),
  connected(false)
{
  port_info.second.client_ports.push_back(this);
}

tNetworkPortInfoClient::~tNetworkPortInfoClient()
{}

void tNetworkPortInfoClient::CheckSubscription()
{
  core::tAbstractPort* abstract_port = NetworkPortInfo().GetAnnotated<core::tAbstractPort>();
  if ((!abstract_port) || (!data_ports::IsDataFlowType(abstract_port->GetDataType())))  // RPC ports do not need a subscription
  {
    return;
  }

  data_ports::common::tAbstractDataPort* port = static_cast<data_ports::common::tAbstractDataPort*>(abstract_port);
  FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Checking subscription of ", *port);

  // Determine necessary subscription parameters
  assert(used_by_connectors.size());
  tDynamicConnectionData new_connection_data;
  static_cast<tDynamicConnectorParameters&>(new_connection_data) = used_by_connectors[0]->DynamicParameters();
  new_connection_data.strategy = port->IsInputPort() ? 0 : port->GetStrategy();
  for (auto it = used_by_connectors.begin() + 1; it < used_by_connectors.end(); ++it)
  {
    new_connection_data.high_priority |= (*it)->DynamicParameters().high_priority;
    new_connection_data.minimal_update_interval = std::min(new_connection_data.minimal_update_interval, (*it)->DynamicParameters().minimal_update_interval);
  }

  rrlib::serialization::tOutputStream& stream = NetworkPortInfo().GetRemoteRuntime().GetPrimaryConnection()->CurrentWriteStream();
  if (connected && new_connection_data == NetworkPortInfo().current_dynamic_connection_data)
  {
    return;
  }
  if (stream.GetTargetInfo().revision == 0)
  {
    if (!used_by_connectors[0]->StaticParameters().server_side_conversion.NoConversion())
    {
      throw std::runtime_error("Remote conversions not supported in initial TCP protocol");
    }

    // Legacy connect
    tLegacySubscribeMessage::Serialize(true, true, stream, NetworkPortInfo().remote_port_handle, new_connection_data.strategy,
                                       false, new_connection_data.GetMinimalUpdateIntervalInMilliseconds(), port->GetHandle(), message_flags::tDataEncoding::cBINARY_ENCODING);
    connected = true;
  }
  else if (!connected)
  {
    tStaticNetworkConnectorParameters static_parameters(used_by_connectors[0]->StaticParameters(), NetworkPortInfo().remote_port_handle);
    tConnectPortsMessage::Serialize(false, true, stream, port->GetHandle(), port->IsInputPort());
    stream << static_parameters << new_connection_data;
    tConnectPortsMessage::FinishMessage(stream);
    connected = true;
  }
  else
  {
    // Update connection
    tUpdateConnectionMessage::Serialize(true, true, stream, NetworkPortInfo().connection_handle, new_connection_data.minimal_update_interval, new_connection_data.high_priority, new_connection_data.strategy);
  }
  NetworkPortInfo().current_dynamic_connection_data = new_connection_data;
}

void tNetworkPortInfoClient::OnManagedDelete()
{
  // Remove from remote port vector
  {
    auto remote_port_info = network_port_info.remote_runtime.remote_port_map.find(network_port_info.remote_port_handle);
    if (remote_port_info != network_port_info.remote_runtime.remote_port_map.end())
    {
      for (auto it = remote_port_info->second.client_ports.begin(); it != remote_port_info->second.client_ports.end(); ++it)
      {
        if ((*it) == this)
        {
          remote_port_info->second.client_ports.erase(it);
          goto remove_done;
        }
      }
    }
    FINROC_LOG_PRINT(WARNING, "Client port not in list (programming error)");
remove_done:
    ;
  }

  // Possibly unsubscribe
  if (NetworkPortInfo().GetRemoteRuntime().IsReady() && NetworkPortInfo().GetRemoteRuntime().GetPrimaryConnection())
  {
    rrlib::serialization::tOutputStream& stream = NetworkPortInfo().GetRemoteRuntime().GetPrimaryConnection()->CurrentWriteStream();
    if (!connected)
    {
      return;
    }
    if (stream.GetTargetInfo().revision == 0)
    {
      tLegacyUnsubscribeMessage::Serialize(true, true, stream, NetworkPortInfo().remote_port_handle);
    }
    else
    {
      tDisconnectPortsMessage::Serialize(true, true, stream, NetworkPortInfo().connection_handle);
    }
  }

  connected = false;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
