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
/*!\file    plugins/network_transport/generic_protocol/tNetworkTransportPlugin.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-06
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkTransportPlugin.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tUriConnector.h"
#include "plugins/network_transport/generic_protocol/tNetworkServerPort.h"

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
typedef core::tFrameworkElement::tFlag tFlag;

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_OPERATION_1("Server-side Conversion Operation 1", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_OPERATION_2("Server-side Conversion Operation 2", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_PARAMETER_1("Server-side Conversion Operation 1 Parameter", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_PARAMETER_2("Server-side Conversion Operation 2 Parameter", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_INTERMEDIATE_TYPE("Server-side Conversion Intermediate Type", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_DESTINATION_TYPE("Server-side Conversion Destination Type", rrlib::rtti::tDataType<std::string>(), true);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_MINIMAL_UPDATE_INTERVAL("Minimal Update Interval", rrlib::rtti::tDataType<rrlib::time::tDuration>(), false);
const rrlib::rtti::tParameterDefinition tNetworkTransportPlugin::cPARAMETER_HIGH_PRIORITY("High Priority", rrlib::rtti::tDataType<bool>(), false);
const std::array<rrlib::rtti::tParameterDefinition, 8> cPARAMETER_DEFINITION_ARRAY =
{
  tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_OPERATION_1, tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_OPERATION_2,
  tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_PARAMETER_1, tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_PARAMETER_2,
  tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_INTERMEDIATE_TYPE, tNetworkTransportPlugin::cPARAMETER_REMOTE_CONVERSION_DESTINATION_TYPE,
  tNetworkTransportPlugin::cPARAMETER_MINIMAL_UPDATE_INTERVAL, tNetworkTransportPlugin::cPARAMETER_HIGH_PRIORITY
};
const rrlib::rtti::tConstParameterDefinitionRange tNetworkTransportPlugin::cPARAMETER_DEFINITIONS(cPARAMETER_DEFINITION_ARRAY.begin(), cPARAMETER_DEFINITION_ARRAY.end());


tNetworkTransportPlugin::tNetworkTransportPlugin(const char* name, const char* scheme_name) :
  tBase(name, scheme_name, cPARAMETER_DEFINITIONS),
  par_max_not_acknowledged_packets_bulk(this, "Maximum non-acknowledged primary connection packets", 3, data_ports::tBounds<uint32_t>(1, 40, true)),
  par_critical_ping_threshold(this, "Critical Ping Threshold", std::chrono::milliseconds(1500), data_ports::tBounds<rrlib::time::tDuration>(std::chrono::milliseconds(50), std::chrono::seconds(20))),
  plugin_elements_root(nullptr)
{
}

bool tNetworkTransportPlugin::Create(core::tAbstractPort& owner_port, const rrlib::uri::tURI& uri, const rrlib::uri::tURIElements& parsed_uri, const core::tUriConnectOptions& connect_options)
{
  auto connector = new tUriConnector(owner_port, uri, parsed_uri, connect_options, *this);
  connector->Publish();
  return connector;
}

void tNetworkTransportPlugin::CreateConnection(tNetworkConnector& connector, tRemoteRuntime& remote_runtime, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info)
{
  // Check whether a client port exists
  for (tNetworkPortInfoClient * client_port : port_info.second.client_ports)
  {

    // Is there a connector whose port(s) can be shared completely
    assert(client_port->used_by_connectors.size());
    core::tAbstractPort* port = client_port->GetPort();
    for (tNetworkConnector * other_connector : client_port->used_by_connectors)
    {
      if (connector.StaticParameters().server_side_conversion == other_connector->StaticParameters().server_side_conversion && (connector.LocalConversionOperations().Size() == 0 || connector.LocalConversionOperations() == other_connector->LocalConversionOperations()))
      {
        client_port->used_by_connectors.push_back(&connector);
        connector.temporary_connector_port = other_connector->temporary_connector_port;
        if (connector.LocalConversionOperations().Size())
        {
          connector.temporary_conversion_port = other_connector->temporary_conversion_port;
          connector.temporary_conversion_port->ConnectTo(connector.OwnerPort(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR);
        }
        else
        {
          connector.temporary_connector_port->GetPort()->ConnectTo(connector.OwnerPort(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR);
        }
        connector.UpdateStatus(core::tUriConnector::tStatus::CONNECTED);
        return;
      }
    }

    // Is there a connector whose port but not conversion can be shared
    for (tNetworkConnector * other_connector : client_port->used_by_connectors)
    {
      if (connector.StaticParameters().server_side_conversion == other_connector->StaticParameters().server_side_conversion)
      {
        client_port->used_by_connectors.push_back(&connector);
        connector.temporary_connector_port = other_connector->temporary_connector_port;
        connector.temporary_conversion_port.reset(new data_ports::tGenericPort(remote_runtime.client_ports, rrlib::uri::tURI(connector.StaticParameters().server_port_id.path).ToString(), connector.OwnerPort().GetDataType(), tFlag::ACCEPTS_DATA | tFlag::PUSH_STRATEGY | tFlag::EMITS_DATA | (port->IsOutputPort() ? tFlag::OUTPUT_PORT : tFlag::PORT)), core::tPortWrapperBase::tDeleter());
        connector.temporary_conversion_port->ConnectTo(connector.temporary_connector_port->GetPort(), core::tConnectOptions(connector.LocalConversionOperations(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR));
        connector.temporary_conversion_port->ConnectTo(connector.OwnerPort(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR);
        connector.UpdateStatus(core::tUriConnector::tStatus::CONNECTED);
        return;
      }
    }
  }

  // Okay, create a new client port
  rrlib::rtti::tType data_type;
  if (!connector.StaticParameters().server_side_conversion.NoConversion())
  {
    data_type = rrlib::rtti::tType::FindType(connector.StaticParameters().server_side_conversion.destination_type);
    if (!data_type)
    {
      FINROC_LOG_PRINT(WARNING, "No local data type '", connector.StaticParameters().server_side_conversion.destination_type, "' -> no connection created.");
      connector.UpdateStatus(core::tUriConnector::tStatus::ERROR);
      return;
    }
  }
  else
  {
    data_type = port_info.second.static_info.type->GetLocalDataType();
    if (!data_type)
    {
      FINROC_LOG_PRINT(WARNING, "No local data type '", port_info.second.static_info.type->GetName(), "' -> no connection created.");
      connector.UpdateStatus(core::tUriConnector::tStatus::ERROR);
      return;
    }
  }

  core::tPortWrapperBase created_port;
  tNetworkPortInfo* network_port_info = nullptr;
  bool output_port = port_info.second.static_info.flags.Get(tFlag::OUTPUT_PORT);
  if (data_ports::IsDataFlowType(data_type))
  {
    data_ports::tGenericPort connector_port(remote_runtime.client_ports, rrlib::uri::tURI(connector.StaticParameters().server_port_id.path).ToString(), data_type, tFlag::VOLATILE | tFlag::NETWORK_ELEMENT | (output_port ? data_ports::cDEFAULT_OUTPUT_PORT_FLAGS : data_ports::cDEFAULT_INPUT_PORT_FLAGS));
    created_port = connector_port;
    network_port_info = new tNetworkPortInfo(remote_runtime, connector_port.GetHandle(), port_info.first, port_info.second.dynamic_info.strategy, *created_port.GetWrapped(), created_port.GetWrapped()->GetHandle());
    connector_port.AddPortListenerForPointer(*network_port_info);
    assert(local_runtime_info);
    connector_port.SetPullRequestHandler(&remote_runtime);
  }
  else if (rpc_ports::IsRPCType(data_type) && (!output_port))
  {
    tNetworkServerPort* port = new tNetworkServerPort(remote_runtime, remote_runtime.client_ports, rrlib::uri::tURI(connector.StaticParameters().server_port_id.path).ToString(), data_type, tFlag::NETWORK_ELEMENT | tFlag::VOLATILE);
    created_port = port;
    network_port_info = new tNetworkPortInfo(remote_runtime, port->GetHandle(), port_info.first, port_info.second.dynamic_info.strategy, *created_port.GetWrapped(), created_port.GetWrapped()->GetHandle());
  }
  if (created_port.GetWrapped())
  {
    network_port_info->client_info.reset(new tNetworkPortInfoClient(*network_port_info, port_info));
    network_port_info->client_info->used_by_connectors.push_back(&connector);
    connector.temporary_connector_port.reset(network_port_info->client_info.get(), tNetworkConnector::tClientPortDeleter());

    if (connector.LocalConversionOperations().Size())
    {
      connector.temporary_conversion_port.reset(new data_ports::tGenericPort(remote_runtime.client_ports, rrlib::uri::tURI(connector.StaticParameters().server_port_id.path).ToString(), connector.OwnerPort().GetDataType(), tFlag::ACCEPTS_DATA | tFlag::PUSH_STRATEGY | tFlag::EMITS_DATA | (created_port.GetWrapped()->IsOutputPort() ? tFlag::OUTPUT_PORT : tFlag::PORT)), core::tPortWrapperBase::tDeleter());
      connector.temporary_conversion_port->ConnectTo(connector.temporary_connector_port->GetPort(), core::tConnectOptions(connector.LocalConversionOperations(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR));
      connector.temporary_conversion_port->ConnectTo(connector.OwnerPort(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR);
    }
    else
    {
      connector.temporary_connector_port->GetPort()->ConnectTo(connector.OwnerPort(), core::tConnectionFlag::NON_PRIMARY_CONNECTOR);
    }
    created_port.Init();
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Created remote port ", *created_port.GetWrapped(), " ", port_info.second.id.handle);
    connector.UpdateStatus(core::tUriConnector::tStatus::CONNECTED);
  }
  else
  {
    FINROC_LOG_PRINT(WARNING, "No port created");
    connector.UpdateStatus(core::tUriConnector::tStatus::ERROR);
  }
}

tRemoteRuntime* tNetworkTransportPlugin::GetRemoteRuntime(const std::string& uri_authority)
{
  for (tRemoteRuntime * runtime : connected_runtimes)
  {
    if (runtime->GetName() == uri_authority)
    {
      return runtime;
    }
  }
  return nullptr;
}

void tNetworkTransportPlugin::Init(rrlib::xml::tNode* config_node)
{
  if (!local_runtime_info)
  {
    local_runtime_info.reset(new tLocalRuntimeInfo(*this));
    local_runtime_info->Init(local_runtime_info);
    plugin_elements_root = new core::tFrameworkElement(&core::tRuntimeEnvironment::GetInstance().GetElement(core::tSpecialRuntimeElement::RUNTIME_NODE), this->GetName(), tFlag::NETWORK_ELEMENT);
    core::tFrameworkElementTags::AddTag(*plugin_elements_root, core::tFrameworkElementTags::cHIDDEN_IN_TOOLS);
    plugin_elements_root->Init();
  }
  if (tLocalRuntimeInfo::IsServingStructure())
  {
    OnStartServingStructure();
  }
}

void tNetworkTransportPlugin::OnNewRemotePort(tRemoteRuntime& remote_runtime, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info)
{
  for (auto & connector : network_connectors)
  {
    for (auto & link : port_info.second.static_info.link_data)
    {
      if (connector->StaticParameters().server_port_id.path == link.path &&
          (connector->StaticParameters().server_port_id.authority.length() == 0 || GetRemoteRuntime(connector->StaticParameters().server_port_id.authority) == &remote_runtime))
      {
        CreateConnection(*connector, remote_runtime, port_info);
        if (connector->temporary_connector_port)
        {
          connector->temporary_connector_port->CheckSubscription();
        }
      }
    }
  }
}

void tNetworkTransportPlugin::ProcessLocalRuntimeCallsToSend()
{
  rrlib::concurrent_containers::tQueueFragment<tLocalRuntimeInfo::tPortSendCallEventPointer> calls_to_send = local_runtime_info->incoming_calls_to_send.DequeueAll();
  while (!calls_to_send.Empty())
  {
    tLocalRuntimeInfo::tPortSendCallEventPointer call_event = calls_to_send.PopFront();
    if (call_event->server_port)
    {
      if (call_event->server_port->IsReady())
      {
        call_event->server_port->DoSendCalls();
      }
    }
    else
    {
      assert(call_event->pull_call);
      call_event->pull_call->remote_runtime->SendPullRequest(*call_event->pull_call);
    }
  }
}

void tNetworkTransportPlugin::ProcessLocalRuntimePortDataChanges()
{
  rrlib::concurrent_containers::tQueueFragment<tLocalRuntimeInfo::tPortDataChangeEventPointer> port_changes = local_runtime_info->incoming_port_data_changes.DequeueAll();
  while (!port_changes.Empty())
  {
    tLocalRuntimeInfo::tPortDataChangeEventPointer change_event = port_changes.PopFront();
    tNetworkPortInfo& network_port_info = *change_event->network_port_info;

    // Collect change events in ports  (they are sent by tConnection::SendPendingMessages)
    if ((!network_port_info.deleted))
    {
      // TODO: is this check still necessary?
      if (network_port_info.values_to_send.GetMaxLength() == 0 && network_port_info.client_info) // if we have unlucky timing, subscription is checked afterwards - so do this here as well
      {
        network_port_info.client_info->CheckSubscription();
      }

      if (network_port_info.values_to_send.GetMaxLength() > 0)
      {
        bool old_values_to_send_empty = network_port_info.values_to_send.Size() == 0;
        network_port_info.values_to_send.Enqueue(change_event);
        if (old_values_to_send_empty)
        {
          network_port_info.remote_runtime.GetConnection(network_port_info.current_dynamic_connection_data.high_priority)->EnqueuePortWithDataToSend(network_port_info);
        }
      }
    }
  }
}

void tNetworkTransportPlugin::ProcessLocalRuntimeStructureChanges()
{
  assert(local_runtime_info);

  rrlib::concurrent_containers::tQueueFragment<std::unique_ptr<tLocalRuntimeInfo::tSharedStructureChangeEvent>> incoming_structure_changes_fragment = local_runtime_info->incoming_structure_changes.DequeueAll();
  std::vector<tNetworkPortInfoClient*> check_subscriptions;
  while (!incoming_structure_changes_fragment.Empty())
  {
    // TODO Event that strategy of client port changed -> check subscription

    std::unique_ptr<tLocalRuntimeInfo::tSharedStructureChangeEvent> incoming_structure_change = incoming_structure_changes_fragment.PopFront();
    const tLocalRuntimeInfo::tStructureChangeEvent& event = (*incoming_structure_change->event);

    if (typeid(event) == typeid(tLocalRuntimeInfo::tClientPortStrategyChangeEvent))
    {
      const tLocalRuntimeInfo::tClientPortStrategyChangeEvent& change_event = static_cast<const tLocalRuntimeInfo::tClientPortStrategyChangeEvent&>(event);
      if (change_event.port.IsReady())
      {
        tNetworkPortInfo* info = change_event.port.GetAnnotation<tNetworkPortInfo>();
        if (info && info->GetClientInfo())
        {
          check_subscriptions.push_back(info->GetClientInfo());
        }
      }
    }
    else if (typeid(event) == typeid(tLocalRuntimeInfo::tUriConnectorAddEvent))
    {
      const tLocalRuntimeInfo::tUriConnectorAddEvent& connector_event = static_cast<const tLocalRuntimeInfo::tUriConnectorAddEvent&>(event);
      network_connectors.emplace_back(new tNetworkConnector(connector_event, connector_event.local_conversion));

      // Check whether to subscribe immediately
      tNetworkConnector& connector = *network_connectors.back();
      rrlib::util::tIteratorRange<tRemoteRuntime**> runtime_range;
      tRemoteRuntime* runtime_pointer = nullptr;
      if (connector.StaticParameters().server_port_id.authority.length())
      {
        runtime_pointer = GetRemoteRuntime(connector.StaticParameters().server_port_id.authority);
        runtime_range = rrlib::util::tIteratorRange<tRemoteRuntime**>(&runtime_pointer, &runtime_pointer + (runtime_pointer ? 1 : 0));
      }
      else
      {
        runtime_range = rrlib::util::tIteratorRange<tRemoteRuntime**>(&connected_runtimes[0], &connected_runtimes[0] + connected_runtimes.size());
      }
      for (tRemoteRuntime * runtime : runtime_range)
      {
        for (auto & port_info : runtime->remote_port_map)
        {
          for (auto & link : port_info.second.static_info.link_data)
          {
            if (link.path == connector.StaticParameters().server_port_id.path)
            {
              CreateConnection(connector, *runtime, port_info);
              if (connector.temporary_connector_port)
              {
                check_subscriptions.push_back(connector.temporary_connector_port.get());
              }
              goto process_next_change;
            }
          }
        }
      }
    }
    else if (typeid(event) == typeid(tLocalRuntimeInfo::tUriConnectorChangeEvent))
    {
      const tLocalRuntimeInfo::tUriConnectorChangeEvent& connector_event = static_cast<const tLocalRuntimeInfo::tUriConnectorChangeEvent&>(event);
      for (auto & connector : network_connectors)
      {
        if (connector->UriConnectorAddress() == connector_event.uri_connector_address)
        {
          connector->DynamicParameters() = connector_event.dynamic_parameters;
          if (connector->temporary_connector_port)
          {
            check_subscriptions.push_back(connector->temporary_connector_port.get());
            assert(check_subscriptions.back());
          }
          goto process_next_change;
        }
      }
      FINROC_LOG_PRINT(WARNING, "Connector not found");
    }
    else if (typeid(event) == typeid(tLocalRuntimeInfo::tUriConnectorDeleteEvent))
    {
      const tLocalRuntimeInfo::tUriConnectorDeleteEvent& connector_event = static_cast<const tLocalRuntimeInfo::tUriConnectorDeleteEvent&>(event);
      for (auto it = network_connectors.begin(); it != network_connectors.end(); ++it)
      {
        if ((*it)->UriConnectorAddress() == connector_event.uri_connector_address)
        {
          bool unique_subscriber = (*it)->temporary_connector_port.unique();
          if (unique_subscriber)
          {
            check_subscriptions.erase(std::remove(check_subscriptions.begin(), check_subscriptions.end(), (*it)->temporary_connector_port.get()), check_subscriptions.end());
            network_connectors.erase(it);
          }
          else
          {
            auto copy = (*it)->temporary_connector_port.get();
            network_connectors.erase(it);
            check_subscriptions.push_back(copy);
          }
          goto process_next_change;
        }
      }
      FINROC_LOG_PRINT(WARNING, "Connector not found");
    }
    else if (typeid(event) == typeid(tLocalRuntimeInfo::tRpcPortDeleteEvent))
    {
      const tLocalRuntimeInfo::tRpcPortDeleteEvent& delete_event = static_cast<const tLocalRuntimeInfo::tRpcPortDeleteEvent&>(event);
      for (tRemoteRuntime * runtime : connected_runtimes)
      {
        runtime->rpc_call_buffer_pools.erase(delete_event.handle);
      }
    }
    else
    {
      const tLocalRuntimeInfo::tStructureChangeEventToPublish& structure_event = static_cast<const tLocalRuntimeInfo::tStructureChangeEventToPublish&>(event);

      // Forward event
      for (tRemoteRuntime * runtime : connected_runtimes)
      {
        if (static_cast<size_t>(runtime->GetDesiredStructureInfo()) >= static_cast<size_t>(structure_event.minimum_relevant_level))
        {
          runtime->PublishStructureChange(structure_event);
        }
      }
    }
process_next_change:
    ;
  }

  // check subscriptions
  std::sort(check_subscriptions.begin(), check_subscriptions.end());
  tNetworkPortInfoClient* last_checked = nullptr;
  for (tNetworkPortInfoClient * client : check_subscriptions)
  {
    if (client != last_checked)
    {
      client->CheckSubscription();
      last_checked = client;
    }
  }
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
