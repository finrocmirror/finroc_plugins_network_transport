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
/*!\file    plugins/network_transport/generic_protocol/tRemoteRuntime.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tRemoteRuntime.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"
#include "plugins/rpc_ports/internal/tRPCInterfaceTypeInfo.h"
#include "plugins/rpc_ports/tPromise.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfo.h"
#include "plugins/network_transport/generic_protocol/tNetworkTransportPlugin.h"

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

namespace
{

/*!
 * Deserialization scope for RPC calls.
 * Buffer pool is created/provided when needed
 */
class tRPCDeserializationScope : public data_ports::api::tDeserializationScope
{
public:
  tRPCDeserializationScope(core::tFrameworkElement::tHandle local_port_handle,
                           std::map<core::tFrameworkElement::tHandle, std::unique_ptr<data_ports::standard::tMultiTypePortBufferPool>>& rpc_call_buffer_pools) :
    tDeserializationScope(),
    local_port_handle(local_port_handle),
    rpc_call_buffer_pools(rpc_call_buffer_pools)
  {}

private:
  virtual data_ports::standard::tMultiTypePortBufferPool& ObtainBufferPool() override
  {
    auto it = rpc_call_buffer_pools.find(local_port_handle);
    if (it == rpc_call_buffer_pools.end())
    {
      it = rpc_call_buffer_pools.insert(std::pair<core::tFrameworkElement::tHandle, std::unique_ptr<data_ports::standard::tMultiTypePortBufferPool>>(
                                          local_port_handle, std::unique_ptr<data_ports::standard::tMultiTypePortBufferPool>(new data_ports::standard::tMultiTypePortBufferPool()))).first;
    }
    return *(it->second);
  }

  core::tFrameworkElement::tHandle local_port_handle;
  std::map<core::tFrameworkElement::tHandle, std::unique_ptr<data_ports::standard::tMultiTypePortBufferPool>>& rpc_call_buffer_pools;
};

class tServerSideConversionAnnotation : public core::tAnnotation
{
public:

  tServerSideConversionAnnotation(const tServerSideConversionInfo& conversion_info) :
    conversion_info(conversion_info)
  {}

  const tServerSideConversionInfo conversion_info;
};

/*!
 * \param server_port_map Server port map remote runtime
 * \param connection_handle Connection handle (== client port handle)
 * \param server_port Handle is from message to server (connection_handle != port handle)
 * \return Network connector port associated to connector handle (abstract port and tNetworkPortInfo pointers)
 */
std::pair<core::tAbstractPort*, tNetworkPortInfo*> GetNetworkConnectorPort(const std::map<tFrameworkElementHandle, tNetworkPortInfo*>& server_port_map, tHandle connection_handle, bool server_port)
{
  typedef std::pair<core::tAbstractPort*, tNetworkPortInfo*> tReturn;
  if (server_port)
  {
    auto it = server_port_map.find(connection_handle);
    if (it != server_port_map.end())
    {
      return tReturn(it->second->GetAnnotated<core::tAbstractPort>(), it->second);
    }
  }
  else
  {
    core::tAbstractPort* port = core::tRuntimeEnvironment::GetInstance().GetPort(connection_handle);
    if (port)
    {
      tNetworkPortInfo* info = port->GetAnnotation<tNetworkPortInfo>();
      if (info)
      {
        return tReturn(port, info);
      }
    }
  }
  return tReturn(nullptr, nullptr);
}

}


tRemoteRuntime::tRemoteRuntime(tNetworkTransportPlugin& network_transport, const std::shared_ptr<tConnection>& primary_connection, core::tFrameworkElement& parent, const std::string& name) :
  tFrameworkElement(&parent, name, tFlag::NETWORK_ELEMENT),
  network_transport(network_transport),
  shared_connection_info(primary_connection->shared_connection_info),
  client_ports(new core::tFrameworkElement(this, "Client Ports", tFlag::NETWORK_ELEMENT | tFlag::AUTO_RENAME)),
  server_ports(new core::tFrameworkElement(this, "Server Ports", tFlag::NETWORK_ELEMENT | tFlag::AUTO_RENAME)),
  server_port_map(),
  remote_port_map(),
  not_ready_calls(),
  calls_awaiting_response(),
  next_call_id(0),
  pull_calls_awaiting_response(),
  rpc_call_buffer_pools()
{
  connections[0] = primary_connection;
  FINROC_LOG_PRINT(DEBUG, "Connected to " + GetName());
}

tRemoteRuntime::~tRemoteRuntime()
{}

bool tRemoteRuntime::AddConnection(std::shared_ptr<tConnection> connection, bool primary_connection)
{
  if ((!primary_connection) && (!connections[0]))
  {
    FINROC_LOG_PRINT(WARNING, "Primary connection must be added first");
    return false;
  }

  size_t index = primary_connection ? 0 : 1;
  if (connections[index])
  {
    return false;
  }
  connections[index] = connection;

  if ((!primary_connection) && (connections[0]->shared_connection_info != connections[1]->shared_connection_info))
  {
    connections[1]->shared_connection_info = connections[0]->shared_connection_info;
  }

  return true;
}

void tRemoteRuntime::AddRemotePort(network_transport::runtime_info::tRemoteFrameworkElementInfo& info)
{
  if (info.static_info.link_count == 0)
  {
    FINROC_LOG_PRINT(WARNING, "Remote shared port has no links. Ignoring.");
    return;
  }
  if (!info.static_info.type)
  {
    FINROC_LOG_PRINT(WARNING, "Remote shared port '", info.static_info.link_data[0].name, "' has unknown type. Ignoring.");
    return;
  }
  if (remote_port_map.find(info.id.handle) != remote_port_map.end())
  {
    FINROC_LOG_PRINT(WARNING, "Received info on remote shared port '", info.static_info.link_data[0].name, "' twice.");
    return;
  }
  auto result = remote_port_map.emplace(info.id.handle, info);
  if (!result.second)
  {
    throw std::runtime_error("Remote port already in remote_port_map");
  }
  FINROC_LOG_PRINT(DEBUG, "Received remote shared port info: ", info.static_info.link_data[0].path);
  network_transport.OnNewRemotePort(*this, *result.first);
}

void tRemoteRuntime::OnInitialization()
{
  network_transport.connected_runtimes.push_back(this);
}

void tRemoteRuntime::OnManagedDelete()
{
  {
    auto& vector = network_transport.connected_runtimes;
    vector.erase(std::remove(vector.begin(), vector.end(), this), vector.end());
  }

  for (auto & connection : connections)
  {
    if (connection)
    {
      connection->Close();
      connection.reset();
    }
  }
  not_ready_calls.clear();
  calls_awaiting_response.clear();

  // Update connectors (important: called before children are deleted)
  for (auto & port_info : remote_port_map)
  {
    for (tNetworkPortInfoClient * client_port : port_info.second.client_ports)
    {
      for (tNetworkConnector * connector : client_port->used_by_connectors)
      {
        connector->UpdateStatus(core::tUriConnector::tStatus::DISCONNECTED);
        connector->temporary_connector_port.reset();
        connector->temporary_conversion_port.reset();
      }
      client_port->used_by_connectors.clear();
    }
  }
}

data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> tRemoteRuntime::OnPullRequest(data_ports::tGenericPort& origin)
{
  return network_transport.LocalRuntimeInfo()->OnPullRequest(origin, *this);
}

void tRemoteRuntime::PortDeleted(tNetworkPortInfo& deleted_port)
{
  for (auto & connection : connections)
  {
    if (connection)
    {
      auto& vector = connection->ports_with_data_to_send;
      vector.erase(std::remove(vector.begin(), vector.end(), &deleted_port), vector.end());
    }
  }

  if (deleted_port.IsServerPort())
  {
    size_t erased = server_port_map.erase(deleted_port.GetConnectionHandle());
    if (!erased)
    {
      FINROC_LOG_PRINT(ERROR, "Deleted server port was not im map (This is a programming error)");
    }
  }
}

bool tRemoteRuntime::ProcessMessage(tOpCode opcode, rrlib::serialization::tMemoryBuffer& buffer, tConnection& connection)
{
  FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Processing message ", make_builder::GetEnumString(opcode));
  rrlib::serialization::tInputStream stream(buffer, shared_connection_info->input_stream_prototype);

  if (opcode == tOpCode::PORT_VALUE_CHANGE || opcode == tOpCode::SMALL_PORT_VALUE_CHANGE || opcode == tOpCode::SMALL_PORT_VALUE_CHANGE_WITHOUT_TIMESTAMP)
  {
    tPortValueChange message;
    message.Deserialize(stream, false);
    uint8_t flags = message.Get<1>();
    message_flags::tDataEncoding encoding = static_cast<message_flags::tDataEncoding>(flags & 0x3);
    std::pair<core::tAbstractPort*, tNetworkPortInfo*> port = GetNetworkConnectorPort(server_port_map, message.Get<0>(), flags & message_flags::cTO_SERVER);
    if (port.first && port.first->IsReady() && data_ports::IsDataFlowType(port.first->GetDataType()))
    {
      tNetworkPortInfo::current_publishing_network_port = port.second;
      data_ports::tGenericPort generic_port = data_ports::tGenericPort::Wrap(*port.first);

      bool another_value = false;
      do
      {
        rrlib::time::tTimestamp timestamp = rrlib::time::cNO_TIME;
        data_ports::tChangeStatus change_type;
        stream >> change_type;
        if (opcode != tOpCode::SMALL_PORT_VALUE_CHANGE_WITHOUT_TIMESTAMP)
        {
          stream >> timestamp;
        }
        data_ports::tPortDataPointer<rrlib::rtti::tGenericObject> buffer = generic_port.GetUnusedBuffer();
        buffer.SetTimestamp(timestamp);
        if (encoding == message_flags::tDataEncoding::cBINARY_COMPRESSED_ENCODING)
        {
          char compression_format_buffer[100];
          if (stream.ReadString(compression_format_buffer, true) >= sizeof(compression_format_buffer))
          {
            FINROC_LOG_PRINT(WARNING, "Compression format string exceeds max length");
            return false;
          }
          if (compression_format_buffer[0])
          {
            size_t data_size = stream.ReadInt();
            size_t data_end_position = stream.GetAbsoluteReadPosition() + data_size;
            FINROC_LOG_PRINT(WARNING, "Decompressing data from network failed: finroc_plugins_data_compression is superseded by rrlib_rtti_conversion");
            stream.Seek(data_end_position);
          }
          else
          {
            buffer->Deserialize(stream);
            generic_port.BrowserPublish(buffer, false, change_type);
          }
        }
        else
        {
          buffer->Deserialize(stream, static_cast<rrlib::serialization::tDataEncoding>(encoding));
          generic_port.BrowserPublish(buffer, false, change_type);
        }
        another_value = stream.ReadBoolean();
      }
      while (another_value);

      tNetworkPortInfo::current_publishing_network_port = nullptr;
      message.FinishDeserialize(stream);
      connection.received_data_after_last_connect = true;
    }
  }
  else if (opcode == tOpCode::RPC_CALL)
  {
    tRPCCall message;
    message.Deserialize(stream, false);
    rpc_ports::tCallType call_type = message.Get<1>();

    const runtime_info::tRemoteType& remote_rpc_interface_type = stream.ReadRegisterEntry<runtime_info::tRemoteType>();
    uint8_t function_index;
    stream >> function_index;
    rrlib::rtti::tType rpc_interface_type = remote_rpc_interface_type.GetLocalDataType();
    if (!rpc_interface_type)
    {
      FINROC_LOG_PRINT(ERROR, "Remote type ", remote_rpc_interface_type.GetName(), " is not known here. Ignoring call.");
      return false;
    }

    const rpc_ports::internal::tRPCInterfaceTypeInfo* type_info = rpc_ports::internal::tRPCInterfaceTypeInfo::Get(rpc_interface_type);
    if ((!type_info) || (!rpc_ports::IsRPCType(rpc_interface_type)))
    {
      FINROC_LOG_PRINT(ERROR, "Type ", rpc_interface_type.GetName(), " is no RPC type. Ignoring call.");
      return false;
    }
    FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Received ", make_builder::GetEnumString(call_type));

    if (call_type == rpc_ports::tCallType::RPC_MESSAGE || call_type == rpc_ports::tCallType::RPC_REQUEST)
    {
      core::tAbstractPort* port = core::tRuntimeEnvironment::GetInstance().GetPort(message.Get<0>());
      if (port && rpc_interface_type == port->GetDataType())
      {
        tRPCDeserializationScope deserialization_scope(message.Get<0>(), rpc_call_buffer_pools);
        if (call_type == rpc_ports::tCallType::RPC_MESSAGE)
        {
          type_info->DeserializeMessage(stream, static_cast<rpc_ports::internal::tRPCPort&>(*port), function_index);
        }
        else
        {
          type_info->DeserializeRequest(stream, static_cast<rpc_ports::internal::tRPCPort&>(*port), function_index, *this);
        }
      }
    }
    else // type is RPC response
    {
      tCallId call_id;
      stream >> call_id;

      tCallPointer call_awaiting_this_response;
      for (auto it = calls_awaiting_response.begin(); it != calls_awaiting_response.end(); ++it)
      {
        if (it->second->GetCallId() == call_id)
        {
          call_awaiting_this_response = std::move(it->second);
          calls_awaiting_response.erase(it);
          break;
        }
      }
      FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Call awaiting: ", call_awaiting_this_response.get());
      if (call_awaiting_this_response)
      {
        core::tAbstractPort* port = core::tRuntimeEnvironment::GetInstance().GetPort(call_awaiting_this_response->GetLocalPortHandle());
        if (port)
        {
          tRPCDeserializationScope deserialization_scope(call_awaiting_this_response->GetLocalPortHandle(), rpc_call_buffer_pools);
          type_info->DeserializeResponse(stream, function_index, *this, call_awaiting_this_response.get());
          message.FinishDeserialize(stream);
          return false;
        }
      }
      call_awaiting_this_response.reset();
      type_info->DeserializeResponse(stream, function_index, *this, call_awaiting_this_response.get());
    }
    message.FinishDeserialize(stream);
  }
  else if (opcode == tOpCode::PULLCALL)
  {
    tPullCall message;
    message.Deserialize(stream);
    uint8_t flags = message.Get<2>();
    message_flags::tDataEncoding encoding = static_cast<message_flags::tDataEncoding>(flags & 0x3);
    std::pair<core::tAbstractPort*, tNetworkPortInfo*> port = GetNetworkConnectorPort(server_port_map, message.Get<0>(), flags & message_flags::cTO_SERVER);
    rrlib::serialization::tOutputStream& write_stream = GetConnection(flags & message_flags::cHIGH_PRIORITY)->CurrentWriteStream();
    if (port.first && port.first->IsReady() && data_ports::IsDataFlowType(port.first->GetDataType()))
    {
      tPullCallReturn::Serialize(false, true, write_stream, message.Get<1>(), false);
      data_ports::tGenericPort data_port = data_ports::tGenericPort::Wrap(*port.first);
      data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> pulled_buffer = data_port.GetPointer(data_ports::tStrategy::PULL_IGNORING_HANDLER_ON_THIS_PORT);
      write_stream << pulled_buffer->GetType() << pulled_buffer.GetTimestamp();
      pulled_buffer->Serialize(write_stream, static_cast<rrlib::serialization::tDataEncoding>(encoding));
      tPullCallReturn::FinishMessage(write_stream);
    }
    else
    {
      tPullCallReturn::Serialize(true, true, write_stream, message.Get<1>(), true);
    }
  }
  else if (opcode == tOpCode::PULLCALL_RETURN)
  {
    tPullCallReturn message;
    message.Deserialize(stream, false);

    for (auto it = pull_calls_awaiting_response.begin(); it != pull_calls_awaiting_response.end(); ++it)
    {
      if (it->call_id == message.Get<0>())
      {
        bool failed = message.Get<1>();
        core::tAbstractPort* port = core::tRuntimeEnvironment::GetInstance().GetPort(it->local_port_handle);
        if ((!failed) && port && port->IsReady() && data_ports::IsDataFlowType(port->GetDataType()))
        {
          rrlib::rtti::tType data_type;
          rrlib::time::tTimestamp timestamp;
          stream >> data_type >> timestamp;

          data_ports::tGenericPort data_port = data_ports::tGenericPort::Wrap(*port);
          data_ports::tPortDataPointer<rrlib::rtti::tGenericObject> pulled_buffer = data_port.GetUnusedBuffer();
          if (pulled_buffer->GetType() != data_type)
          {
            FINROC_LOG_PRINT(WARNING, "Port data pulled via ", port, " has invalid type.");
            it->promise->SetException(rpc_ports::tFutureStatus::INVALID_DATA_RECEIVED);
          }
          else
          {
            pulled_buffer.SetTimestamp(timestamp);
            pulled_buffer->Deserialize(stream);
            message.FinishDeserialize(stream);
            it->promise->SetValue(std::move(pulled_buffer));
          }
        }
        else
        {
          it->promise->SetException(rpc_ports::tFutureStatus::NO_CONNECTION);
        }
        pull_calls_awaiting_response.erase(it); // remove pull call from list
        break;
      }
    }
  }
  else if (opcode == tOpCode::UPDATE_CONNECTION)
  {
    tUpdateConnectionMessage message;
    message.Deserialize(stream);

    // Get server port
    std::pair<core::tAbstractPort*, tNetworkPortInfo*> port = GetNetworkConnectorPort(server_port_map, message.Get<0>(), true);
    if (port.first && port.first->IsReady() && data_ports::IsDataFlowType(port.first->GetDataType()))
    {
      tDynamicConnectionData data;
      data.minimal_update_interval = message.Get<1>();
      data.high_priority = message.Get<2>();
      data.strategy = message.Get<3>();
      port.second->SetServerSideDynamicConnectionData(data);
      bool push_strategy = data.strategy > 0;
      data_ports::common::tAbstractDataPort& data_port = static_cast<data_ports::common::tAbstractDataPort&>(*port.first);
      if (data_port.PushStrategy() != push_strategy)
      {
        // flags need to be changed
        rrlib::thread::tLock lock(GetStructureMutex(), false);
        if (lock.TryLock())
        {
          if (data_port.PushStrategy() != push_strategy)
          {
            data_port.SetPushStrategy(push_strategy);
          }
        }
        else
        {
          return true; // We could not obtain lock - try again later
        }
      }
    }
    else
    {
      FINROC_LOG_PRINT(WARNING, "Cannot find connection to update (requested handle: ", message.Get<0>(), ")");
    }
  }

  else if (opcode == tOpCode::CONNECT_PORTS)
  {
    tConnectPortsMessage message;
    message.Deserialize(stream, false);

    try
    {
      // Get or create server port
      auto it = server_port_map.find(message.Get<0>());
      if (it != server_port_map.end())
      {
        throw std::runtime_error("Connection handle already occupied. Ignoring CONNECT_PORTS message.");
      }
      else
      {
        rrlib::thread::tLock lock(GetStructureMutex(), false);
        if (lock.TryLock())
        {
          bool publish_connection = message.Get<1>();
          bool tool_connection = GetDesiredStructureInfo() != runtime_info::tStructureExchange::SHARED_PORTS;

          // Read subscription data
          tStaticNetworkConnectorParameters static_subscription_parameters;
          tDynamicConnectionData dynamic_connection_data;
          stream >> static_subscription_parameters >> dynamic_connection_data;
          message.FinishDeserialize(stream);

          // Create server port
          core::tAbstractPort* port = core::tRuntimeEnvironment::GetInstance().GetPort(static_subscription_parameters.server_port_id);
          if ((!port) || (!port->IsReady()))
          {
            throw std::runtime_error("Port for subscription not available");
          }

          tFlags flags = tFlag::NETWORK_ELEMENT | tFlag::VOLATILE;
          if ((!tool_connection) && port->IsOutputPort() && publish_connection)
          {
            throw std::runtime_error("Cannot publish to output ports with basic (SHARED_PORTS) connection");
          }
          if (!publish_connection)
          {
            flags |= tFlag::ACCEPTS_DATA; // create input port
          }
          else
          {
            flags |= tFlag::OUTPUT_PORT | tFlag::EMITS_DATA | tFlag::NO_INITIAL_PUSHING; // create output io port
          }
          if (tool_connection)
          {
            flags |= tFlag::TOOL_PORT;
          }
          if (dynamic_connection_data.strategy > 0)
          {
            flags |= tFlag::PUSH_STRATEGY;
          }

          std::string port_name = rrlib::uri::tURI(port->GetPath()).ToString() + (publish_connection ? " (Publish)" : " (Subscribe)");
          core::tAbstractPort* port_to_connect_to = port;
          if (!static_subscription_parameters.server_side_conversion.NoConversion())
          {
            port_to_connect_to = nullptr;

            // Check whether port already exists
            if (!publish_connection)
            {
              for (auto it = port->OutgoingConnectionsBegin(); it != port->OutgoingConnectionsEnd(); ++it)
              {
                tServerSideConversionAnnotation* info = it->Destination().GetAnnotation<tServerSideConversionAnnotation>();
                if (info && info->conversion_info == static_subscription_parameters.server_side_conversion)
                {
                  port_to_connect_to = &it->Destination();
                }
              }
            }
            else
            {
              for (auto it = port->IncomingConnectionsBegin(); it != port->IncomingConnectionsEnd(); ++it)
              {
                tServerSideConversionAnnotation* info = it->Source().GetAnnotation<tServerSideConversionAnnotation>();
                if (info && info->conversion_info == static_subscription_parameters.server_side_conversion)
                {
                  port_to_connect_to = &it->Source();
                }
              }
            }

            if (!port_to_connect_to)
            {
              // Resolve conversion
              rrlib::rtti::tType destination_type = rrlib::rtti::tType::FindType(static_subscription_parameters.server_side_conversion.destination_type);
              if (!destination_type)
              {
                throw std::runtime_error("Server-side conversion to unknown type" + static_subscription_parameters.server_side_conversion.destination_type);
              }
              size_t size = static_subscription_parameters.server_side_conversion.operation_1.length() == 0 ? 0 : (static_subscription_parameters.server_side_conversion.operation_2.length() == 0 ? 1 : 2);
              rrlib::rtti::tType intermediate_type;
              if (size == 2 || static_subscription_parameters.server_side_conversion.intermediate_type.length())
              {
                intermediate_type = rrlib::rtti::tType::FindType(static_subscription_parameters.server_side_conversion.intermediate_type);
                if (!intermediate_type)
                {
                  throw std::runtime_error("Server-side conversion with unknown type " + static_subscription_parameters.server_side_conversion.intermediate_type);
                }
              }

              const rrlib::rtti::conversion::tRegisteredConversionOperation* operation1 = nullptr;
              const rrlib::rtti::conversion::tRegisteredConversionOperation* operation2 = nullptr;
              if (size >= 1)
              {
                operation1 = &rrlib::rtti::conversion::tRegisteredConversionOperation::Find(static_subscription_parameters.server_side_conversion.operation_1, port->GetDataType(), size == 1 ? destination_type : intermediate_type);
                if (size >= 2)
                {
                  operation2 = &rrlib::rtti::conversion::tRegisteredConversionOperation::Find(static_subscription_parameters.server_side_conversion.operation_2, intermediate_type, destination_type);
                }
              }

              rrlib::rtti::conversion::tConversionOperationSequence conversion;
              if (size == 1)
              {
                conversion = rrlib::rtti::conversion::tConversionOperationSequence(*operation1, intermediate_type);
              }
              else if (size == 2)
              {
                conversion = rrlib::rtti::conversion::tConversionOperationSequence(*operation1, *operation2, intermediate_type);
              }

              // Create and connect new conversion port
              data_ports::tGenericPort created_port(port_name + " to " + destination_type.GetName(), &GetServerPortsElement(), destination_type, tFlag::NETWORK_ELEMENT | tFlag::VOLATILE | tFlag::EMITS_DATA | tFlag::ACCEPTS_DATA | (publish_connection ? (tFlag::OUTPUT_PORT | tFlag::NO_INITIAL_PUSHING) : tFlag::PORT) | (tool_connection ? tFlag::TOOL_PORT : tFlag::PORT));
              if (created_port.ConnectTo(port, core::tConnectOptions(conversion, core::tConnectionFlag::NON_PRIMARY_CONNECTOR | (publish_connection ? core::tConnectionFlag::DIRECTION_TO_DESTINATION : core::tConnectionFlag::DIRECTION_TO_SOURCE))))
              {
                created_port.GetWrapped()->EmplaceAnnotation<tServerSideConversionAnnotation>(static_subscription_parameters.server_side_conversion);
                created_port.Init();
                port_to_connect_to = created_port.GetWrapped();
              }
              else
              {
                created_port.ManagedDelete();
                throw std::runtime_error("Conversion could not be applied");
              }
            }
          }

          data_ports::tGenericPort created_port(port_name, &GetServerPortsElement(), &GetServerPortsElement(), port_to_connect_to->GetDataType(), flags);
          tNetworkPortInfo* network_port_info = new tNetworkPortInfo(*this, message.Get<0>(), message.Get<0>(), dynamic_connection_data.strategy, *created_port.GetWrapped(), port->GetHandle());
          network_port_info->SetDesiredEncoding(static_subscription_parameters.server_side_conversion.encoding);
          network_port_info->SetServerSideDynamicConnectionData(dynamic_connection_data);
          created_port.AddPortListenerForPointer(*network_port_info);
          created_port.SetPullRequestHandler(this);
          created_port.Init();
          created_port.ConnectTo(*port_to_connect_to, core::tConnectionFlag::NON_PRIMARY_CONNECTOR | (publish_connection ? core::tConnectionFlag::DIRECTION_TO_DESTINATION : core::tConnectionFlag::DIRECTION_TO_SOURCE));
          server_port_map.emplace(message.Get<0>(), network_port_info);
          FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Created server port ", created_port.GetWrapped());
        }
        else
        {
          return true; // We could not obtain lock - try again later
        }
      }
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(WARNING, "Error connecting ports (notifying client also): ", e.what());
      auto& stream = GetPrimaryConnection()->CurrentWriteStream();
      tConnectPortsErrorMessage::Serialize(false, true, stream, message.Get<0>());
      stream << e.what();
      tConnectPortsErrorMessage::FinishMessage(stream);
    }
  }
  else if (opcode == tOpCode::DISCONNECT_PORTS)
  {
    tDisconnectPortsMessage message;
    message.Deserialize(stream);
    auto it = server_port_map.find(message.Get<0>());
    if (it != server_port_map.end())
    {
      rrlib::thread::tLock lock(GetStructureMutex(), false);
      if (lock.TryLock())
      {
        // delete conversion ports
        core::tAbstractPort& network_port = *it->second->GetAnnotated<core::tAbstractPort>();
        if (network_port.IsOutputPort())
        {
          for (auto connector = network_port.OutgoingConnectionsBegin(); connector != network_port.OutgoingConnectionsEnd(); ++connector)
          {
            tServerSideConversionAnnotation* info = connector->Destination().GetAnnotation<tServerSideConversionAnnotation>();
            if (info && connector->Destination().CountIncomingConnections() == 1)
            {
              connector->Destination().ManagedDelete();
            }
          }
        }
        else
        {
          for (auto connector = network_port.IncomingConnectionsBegin(); connector != network_port.IncomingConnectionsEnd(); ++connector)
          {
            tServerSideConversionAnnotation* info = connector->Source().GetAnnotation<tServerSideConversionAnnotation>();
            if (info && connector->Source().CountOutgoingConnections() == 1)
            {
              connector->Source().ManagedDelete();
            }
          }
        }

        // Delete port (this will also
        network_port.ManagedDelete();
      }
      else
      {
        return true; // We could not obtain lock - try again later
      }
    }
    else
    {
      FINROC_LOG_PRINT(DEBUG_WARNING, "Port for disconnecting not available (", message.Get<0>(), ")");
      return false;
    }
  }
  else if (opcode == tOpCode::TYPE_UPDATE)
  {
    tTypeUpdateMessage message;
    message.Deserialize(stream, false);
    rrlib::rtti::tType type;
    stream >> type;
    stream.ReadShort(); // Discard remote network update time default for data type (legacy)
    message.FinishDeserialize(stream);
  }
  else if (opcode == tOpCode::STRUCTURE_CREATED)
  {
    rrlib::thread::tLock lock(GetStructureMutex(), false);
    if (lock.TryLock())
    {
      tStructureCreatedMessage message;
      message.Deserialize(stream, false);
      network_transport::runtime_info::tRemoteFrameworkElementInfo framework_element_info;
      framework_element_info.id.handle = message.Get<0>();
      stream >> framework_element_info;
      message.FinishDeserialize(stream);
      AddRemotePort(framework_element_info);
    }
    else
    {
      return true; // We could not obtain lock - try again later
    }
  }
  else if (opcode == tOpCode::STRUCTURE_CHANGED)
  {
    rrlib::thread::tLock lock(GetStructureMutex(), false);
    if (lock.TryLock())
    {
      tStructureChangedMessage message;
      message.Deserialize(stream, false);
      if (stream.GetSourceInfo().revision == 0)
      {
        stream.ReadInt(); // new flags (flag changes are obsolete)
      }
      runtime_info::tRemoteFrameworkElementInfo::tDynamicInfo dynamic_info;
      stream >> dynamic_info;
      message.FinishDeserialize(stream);

      auto port_to_change = remote_port_map.find(message.Get<0>());
      if (port_to_change != remote_port_map.end())
      {
        //port_to_change
        port_to_change->second.dynamic_info.strategy = dynamic_info.strategy;
        for (tNetworkPortInfoClient * client_port : port_to_change->second.client_ports)
        {
          data_ports::common::tAbstractDataPort& data_port = static_cast<data_ports::common::tAbstractDataPort&>(*client_port->GetPort());
          data_port.SetPushStrategy(dynamic_info.strategy > 0);
          //data_port.SetMinNetUpdateIntervalRaw(dynamic_info.min_net_update_time);
          //tNetworkPortInfo* network_port_info = data_port.GetAnnotation<tNetworkPortInfo>();
          client_port->NetworkPortInfo().current_dynamic_connection_data.strategy = dynamic_info.strategy;
          client_port->NetworkPortInfo().ChangeStrategy(dynamic_info.strategy);
        }
      }
      else
      {
        FINROC_LOG_PRINT(WARNING, "There is no port to change with handle ", message.Get<0>());
      }
    }
    else
    {
      return true; // We could not obtain lock - try again later
    }
  }
  else if (opcode == tOpCode::STRUCTURE_DELETED)
  {
    rrlib::thread::tLock lock(GetStructureMutex(), false);
    if (lock.TryLock())
    {
      tStructureDeletedMessage message;
      message.Deserialize(stream);

      auto port_to_delete = remote_port_map.find(message.Get<0>());
      if (port_to_delete != remote_port_map.end())
      {
        for (tNetworkPortInfoClient * client_port : port_to_delete->second.client_ports)
        {
          for (tNetworkConnector * connector : client_port->used_by_connectors)
          {
            connector->UpdateStatus(core::tUriConnector::tStatus::DISCONNECTED);
            connector->temporary_connector_port.reset();
            connector->temporary_conversion_port.reset();
          }
        }
        remote_port_map.erase(message.Get<0>());
      }
      else
      {
        FINROC_LOG_PRINT(WARNING, "There is no port to delete with handle ", message.Get<0>());
      }
    }
    else
    {
      return true; // We could not obtain lock - try again later
    }
  }
  else if (opcode == tOpCode::CONNECT_PORTS_ERROR)
  {
    tConnectPortsErrorMessage message;
    message.Deserialize(stream, false);
    std::string error = stream.ReadString();
    message.FinishDeserialize(stream);

    std::pair<core::tAbstractPort*, tNetworkPortInfo*> port = GetNetworkConnectorPort(server_port_map, message.Get<0>(), false);
    if (port.first && port.first->IsReady() && port.second->GetClientInfo())
    {
      FINROC_LOG_PRINT(WARNING, "Could not connect to remote port '", port.second->GetClientInfo()->GetPort()->GetName(), "'. Reason: ", error);
      port.second->GetClientInfo()->connected = false;
      for (tNetworkConnector * connector : port.second->GetClientInfo()->used_by_connectors)
      {
        connector->UpdateStatus(core::tUriConnector::tStatus::ERROR);
        connector->temporary_connector_port.reset();
        connector->temporary_conversion_port.reset();
      }
    }
    else
    {
      FINROC_LOG_PRINT(WARNING, "Received CONNECT_PORTS_ERROR message for unknown connection handle");
    }
  }
  else if (opcode == tOpCode::SUBSCRIBE_LEGACY || opcode == tOpCode::UNSUBSCRIBE_LEGACY)
  {
    FINROC_LOG_PRINT(WARNING, "OpCode ", make_builder::GetEnumString(opcode), " is superseded and no longer served by this peer");
    throw std::runtime_error("Superseded OpCode");
  }
  else
  {
    FINROC_LOG_PRINT(WARNING, "OpCode ", make_builder::GetEnumString(opcode), " is not served by this peer");
    throw std::runtime_error("Invalid OpCode");
  }

  return false;
}

void tRemoteRuntime::ProcessStructurePacket(rrlib::serialization::tInputStream& stream)
{
  try
  {
    const runtime_info::tRemoteType& type = stream.ReadRegisterEntry<runtime_info::tRemoteType>();
    if (type.GetName() != rrlib::rtti::tDataType<std::string>().GetName())
    {
      FINROC_LOG_PRINT(ERROR, "Type encoding does not seem to work");
      return;
    }

    network_transport::runtime_info::tRemoteFrameworkElementInfo info;
    while (stream.Remaining())
    {
      stream >> info.id.handle;
      stream >> info;
      AddRemotePort(info);
    }
  }
  catch (const std::exception& e)
  {
    FINROC_LOG_PRINT(ERROR, "Error processing structure packet:", e);
  }
}

void tRemoteRuntime::PublishStructureChange(const tLocalRuntimeInfo::tStructureChangeEventToPublish& structure_change_event)
{
  if (shared_connection_info->initial_reading_complete && shared_connection_info->initial_writing_complete && shared_connection_info->output_stream_prototype.GetTargetInfo().revision != 0) // do not publish to legacy runtimes
  {
    if (shared_connection_info->initial_structure_writing_complete || structure_change_event.local_handle < shared_connection_info->framework_elements_in_full_structure_exchange_sent_until_handle)
    {
      structure_change_event.WriteToStream(GetPrimaryConnection()->CurrentWriteStream());
    }
  }
}

void tRemoteRuntime::SendCall(tCallPointer& call_to_send, const rrlib::time::tTimestamp& time_now)
{
  if (!call_to_send->ReadyForSending())
  {
    //FINROC_LOG_PRINT(ERROR, "Emplacing ", call_to_send.get());
    not_ready_calls.emplace_back(std::move(call_to_send));
  }
  else
  {
    SendCallImplementation(call_to_send, time_now);
  }
}

void tRemoteRuntime::SendCallImplementation(tCallPointer& call_to_send, const rrlib::time::tTimestamp& time_now)
{
  tConnection& connection = *GetExpressConnection();  // TODO: Should there be a possibility to specify which connection to use RPC calls? problem: message does not

  FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Sending Call ", make_builder::GetEnumString(call_to_send->GetCallType()));
  bool expects_response = call_to_send->ExpectsResponse();
  if (expects_response)
  {
    call_to_send->SetCallId(next_call_id);
    next_call_id++;
  }
  tRPCCall::Serialize(false, true, connection.CurrentWriteStream(), call_to_send->GetRemotePortHandle(), call_to_send->GetCallType());
  call_to_send->GetCall()->Serialize(connection.CurrentWriteStream());
  tRPCCall::FinishMessage(connection.CurrentWriteStream());
  if (expects_response)
  {
    rrlib::time::tDuration timeout = call_to_send->ResponseTimeout();
    calls_awaiting_response.emplace_back(time_now + timeout, std::move(call_to_send));
  }
}

void tRemoteRuntime::SendPendingMessages(const rrlib::time::tTimestamp& time_now)
{
  if ((!GetPrimaryConnection()) || GetPrimaryConnection()->IsClosed())
  {
    return;
  }

  for (auto it = not_ready_calls.begin(); it < not_ready_calls.end();)
  {
    if ((*it)->ReadyForSending())
    {
      tCallPointer call_pointer = std::move(*it);
      SendCallImplementation(call_pointer, time_now);
      it = not_ready_calls.erase(it);
    }
    else
    {
      ++it;
    }
  }
  for (auto it = calls_awaiting_response.begin(); it < calls_awaiting_response.end();)
  {
    if (time_now > it->first) // Did call time out?
    {
      it = calls_awaiting_response.erase(it);
    }
    else
    {
      ++it;
    }
  }
  for (auto it = pull_calls_awaiting_response.begin(); it < pull_calls_awaiting_response.end();)
  {
    if (time_now > it->timeout_time) // Did call time out?
    {
      it = pull_calls_awaiting_response.erase(it);
    }
    else
    {
      ++it;
    }
  }

  for (auto & connection : connections)
  {
    if (connection && (!connection->IsClosed()))
    {
      connection->SendPendingMessages(time_now);
    }
  }
}

void tRemoteRuntime::SendPullRequest(tLocalRuntimeInfo::tPullCallInfo& pull_call_info)
{
  // We do this here, because this is the TCP thread now (and next_call_id is not thread-safe)
  pull_call_info.call_id = next_call_id;
  next_call_id++;
  pull_calls_awaiting_response.push_back(pull_call_info);

  // Send call
  rrlib::serialization::tOutputStream& stream = GetExpressConnection()->CurrentWriteStream();  // pull request is small -> we can always use express connection
  bool legacy = stream.GetTargetInfo().revision == 0;
  uint8_t message_flags = legacy ? message_flags::cBINARY_ENCODING : pull_call_info.message_flags;
  tPullCall::Serialize(true, true, stream, legacy ? pull_call_info.remote_port_handle : pull_call_info.connection_handle, pull_call_info.call_id, message_flags);
  // Send immediately? (pull calls are somewhat outdated -> no); ex-code: this->SendPendingMessages(rrlib::time::Now(true));
}

void tRemoteRuntime::SendResponse(typename tResponseSender::tCallPointer && response_to_send)
{
  if (response_to_send->ReadyForSending())
  {
    rrlib::time::tTimestamp time_now = rrlib::time::Now();
    SendCallImplementation(response_to_send, time_now);
    SendPendingMessages(time_now);
  }
  else
  {
    not_ready_calls.emplace_back(std::move(response_to_send));
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
