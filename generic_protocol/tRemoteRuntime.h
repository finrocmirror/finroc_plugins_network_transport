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
/*!\file    plugins/network_transport/generic_protocol/tRemoteRuntime.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 * \brief   Contains tRemoteRuntime
 *
 * \b tRemoteRuntime
 *
 * Represents a remote runtime environment in this process.
 * It creates a proxy port for each shared port in the remote runtime.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tRemoteRuntime_h__
#define __plugins__network_transport__generic_protocol__tRemoteRuntime_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/tPullRequestHandler.h"
#include "plugins/rpc_ports/internal/tRPCPort.h"
#include "plugins/rpc_ports/internal/tResponseSender.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tConnection.h"

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
class tNetworkPortInfo;
class tNetworkTransportPlugin;
class tNetworkPortInfoClient;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Remote runtime environment
/*!
 * Represents a remote runtime environment in this process.
 * It creates a proxy port for each shared port in the remote runtime.
 *
 * This class is intended to be independent of a specific network transport -
 * with possibly connections from multiple transports.
 */
class tRemoteRuntime : public core::tFrameworkElement, public rpc_ports::internal::tResponseSender, public data_ports::tPullRequestHandler<rrlib::rtti::tGenericObject>
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef typename rpc_ports::internal::tRPCPort::tCallPointer tCallPointer;

  /*!
   * \param network_transport Reference to plugin that owns this object
   * \param primary_connection Primary connection to this runtime
   * \param parent Parent framework element to create this below
   * \param name Unique name of this remote runtime
   */
  tRemoteRuntime(tNetworkTransportPlugin& network_transport, const std::shared_ptr<tConnection>& primary_connection, core::tFrameworkElement& parent, const std::string& name);

  ~tRemoteRuntime();

  /*!
   * Add connection for this remote runtime.
   * Primary connection must be added first.
   * Non-primary connection's shared_connection_info is replaced with primary's if they are not identical
   *
   * \param connection Connection to add (with flags set)
   * \param primary_connection Is primary connection? (otherwise high priority 'express' connection)
   * \return Did this succeed? (fails if there already is a connection for specified priority; may happen if two parts try to connect at the same time - only one connection is kept)
   */
  bool AddConnection(std::shared_ptr<tConnection> connection, bool primary_connection);

  /*!
   * Add remote port specified by provided info to this remote part
   */
  void AddRemotePort(runtime_info::tRemoteFrameworkElementInfo& info);

  /*!
   * \return Structure information to send to remote part
   */
  runtime_info::tStructureExchange GetDesiredStructureInfo() const
  {
    return network_transport::runtime_info::GetStructureExchangeLevel(shared_connection_info->output_stream_prototype);
  }

  /*!
   * \param express_conection_if_available Return express connection if available
   * \return Connection to this remote runtime environment depending on 'express_conection_if_available'.
   */
  const std::shared_ptr<tConnection>& GetConnection(bool express_conection_if_available)
  {
    return express_conection_if_available ? GetExpressConnection() : GetPrimaryConnection();
  }

  /*!
   * \return Express connection to this remote runtime environment. If there is no such connection, returns primary connection.
   */
  const std::shared_ptr<tConnection>& GetExpressConnection()
  {
    return connections[connections[1] ? 1 : 0];
  }

  /*!
   * \return Local runtime info
   */
  tLocalRuntimeInfo& GetLocalRuntimeInfo()
  {
    return shared_connection_info->local_runtime_info;
  }

  /*!
   * \return Primary connection to this remote runtime environment
   */
  const std::shared_ptr<tConnection>& GetPrimaryConnection()
  {
    return connections[0];
  }

  /*!
   * \return Framework element that contains all server ports. Is created if it does not exist.
   */
  tFrameworkElement& GetServerPortsElement()
  {
    return *server_ports;
  }

  /*!
   * \return Reference to network transport plugin this remote runtime belongs to
   */
  tNetworkTransportPlugin& NetworkTransport()
  {
    return network_transport;
  }

  /*!
   * Called when network port is deleted.
   * Removes port from server_port_map
   * Removes port from connections's port_with_data_to_send.
   */
  void PortDeleted(tNetworkPortInfo& deleted_port);

//  /*!
//   * \return Ports that currently have bulk data to send
//   */
//  std::vector<tNetworkPortInfo*>& PortsWithBulkDataToSend()
//  {
//    return ports_with_bulk_data_to_send;
//  }
//
//  /*!
//   * \return Ports that currently have express data to send
//   */
//  std::vector<tNetworkPortInfo*>& PortsWithExpressDataToSend()
//  {
//    return ports_with_express_data_to_send;
//  }

  /*!
   * Process message with specified opcode in provided memory buffer
   *
   * \return Defer message? (in this case, ProcessMessage() will be called again later with the same message)
   */
  bool ProcessMessage(tOpCode opcode, rrlib::serialization::tMemoryBuffer& buffer, tConnection& connection);

  /*!
   * Processes remote structure information/changes
   *
   * \param stream Input stream on packet
   */
  void ProcessStructurePacket(rrlib::serialization::tInputStream& stream);

  /*!
   * Sends structure change to remote part
   *
   * \param structure_change_event Structure change to send
   */
  void PublishStructureChange(const tLocalRuntimeInfo::tStructureChangeEventToPublish& structure_change_event);

//  /*!
//   * Removes connection for this remote part
//   *
//   * \param connection Connection to remove
//   */
//  void RemoveConnection(tConnection& connection);

  /*!
   * Sends specified RPC call to connection partner
   */
  void SendCall(tCallPointer& call_to_send, const rrlib::time::tTimestamp& time_now);

  /*!
   * Sends pending messages in all connections and check whether any timeouts occurred
   * (TODO: Should we rename method because of the latter? Or add a parameter 'check_timeouts'?)
   * Note: Due to overhead otherwise, it is beneficial to do both tasks in one method.
   *
   * \param time_now Current time
   */
  void SendPendingMessages(const rrlib::time::tTimestamp& time_now);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tNetworkTransportPlugin;
  friend class tNetworkPortInfoClient;

  /*! Reference to network transport that this runtime belongs to */
  tNetworkTransportPlugin& network_transport;

  /*! Contains information shared among primary and non-primary connections to a specific peer */
  std::shared_ptr<tConnection::tSharedConnectionInfo> shared_connection_info;

  /*! Framework element that contains all client ports */
  core::tFrameworkElement* client_ports;

  /*! Framework element that contains all server ports */
  core::tFrameworkElement* server_ports;

  /*! Maps connection handle => server port */
  std::map<tFrameworkElementHandle, tNetworkPortInfo*> server_port_map;

  /*! Info on remote port */
  class tRemotePortInfo : public runtime_info::tRemoteFrameworkElementInfo
  {
  public:
    tRemotePortInfo(const runtime_info::tRemoteFrameworkElementInfo& info) : runtime_info::tRemoteFrameworkElementInfo(info)
    {}

    std::vector<tNetworkPortInfoClient*> client_ports;  //!< All client ports (already) created for this port (this list is maintained by tNetworkPortInfoClient class); ports with local conversion can be found via tNetworkPortInfoClient class
  };

  /*! Maps remote port handle => port info */
  std::map<tFrameworkElementHandle, tRemotePortInfo> remote_port_map;

  /*! Ports that currently have express and bulk data to send */
  //std::vector<tNetworkPortInfo*> ports_with_express_data_to_send, ports_with_bulk_data_to_send;

  /*! Calls that were not ready for sending yet */
  std::vector<tCallPointer> not_ready_calls;

  /*! Calls that wait for a response */
  std::vector<std::pair<rrlib::time::tTimestamp, tCallPointer>> calls_awaiting_response;

  /*! Next call id to assign to sent call */
  rpc_ports::internal::tCallId next_call_id;

  /*! Pull calls that wait for a response */
  std::vector<tLocalRuntimeInfo::tPullCallInfo> pull_calls_awaiting_response;

  /*!
   * Contains buffer pools for RPC calls from this connection to specific local RPC ports.
   * Key is handle of local RPC port.
   */
  std::map<core::tFrameworkElement::tHandle, std::unique_ptr<data_ports::standard::tMultiTypePortBufferPool>> rpc_call_buffer_pools;

  /*!
   * Pointers to connections to partner peer.
   * The first entry is the primary connection.
   * The primary connection between two peers exchanges structure, registers, and manages other connections.
   * The second entry is an optional 'express' connection with the same URI scheme.
   */
  std::array<std::shared_ptr<tConnection>, 2> connections;


  virtual void OnInitialization() override;
  virtual void OnManagedDelete() override;
  virtual data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> OnPullRequest(data_ports::tGenericPort& origin) override;

  /*!
   * Sends specified RPC call to connection partner
   *
   * \param call_to_send Call to send (already checked whether it is ready)
   */
  void SendCallImplementation(tCallPointer& call_to_send, const rrlib::time::tTimestamp& time_now);

  /*!
   * Sends pull request to remote peer
   * (May only be called by TCP Thread)
   */
  void SendPullRequest(tLocalRuntimeInfo::tPullCallInfo& pull_call_info);

  virtual void SendResponse(typename tResponseSender::tCallPointer && response_to_send) override;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
