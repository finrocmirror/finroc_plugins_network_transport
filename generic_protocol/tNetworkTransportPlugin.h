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
/*!\file    plugins/network_transport/generic_protocol/tNetworkTransportPlugin.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-06
 *
 * \brief   Contains tNetworkTransportPlugin
 *
 * \b tNetworkTransportPlugin
 *
 * Base class for all plugins that provide a network transport mechanism based on generic protocol in this directory.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tNetworkTransportPlugin_h__
#define __plugins__network_transport__generic_protocol__tNetworkTransportPlugin_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/network_transport/tNetworkTransportPlugin.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tLocalRuntimeInfo.h"
#include "plugins/network_transport/generic_protocol/tUriConnectorData.h"
#include "plugins/network_transport/generic_protocol/tNetworkConnector.h"

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
//! Network transport plugin
/*!
 * Base class for all plugins that provide a network transport mechanism based on generic protocol in this directory.
 *
 * Implementation note: The network protocol is designed for single-threaded operation:
 *                      fields and methods in this class may not be called concurrently ('Create' is the only exception)
 */
class tNetworkTransportPlugin : public network_transport::tNetworkTransportPlugin
{
  typedef network_transport::tNetworkTransportPlugin tBase;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  // Constant parameter definitions
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_OPERATION_1;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_OPERATION_2;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_PARAMETER_1;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_PARAMETER_2;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_INTERMEDIATE_TYPE;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_REMOTE_CONVERSION_DESTINATION_TYPE;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_MINIMAL_UPDATE_INTERVAL;
  static const rrlib::rtti::tParameterDefinition cPARAMETER_HIGH_PRIORITY;
  static const rrlib::rtti::tConstParameterDefinitionRange cPARAMETER_DEFINITIONS;


  /*! Maximum packets to send without acknowledgement in bulk connections */
  tParameter<uint32_t> par_max_not_acknowledged_packets_bulk;

  /*! Critical ping threshold (if this is exceeded ports are notified of disconnect) */
  tParameter<rrlib::time::tDuration> par_critical_ping_threshold;


  /*!
   * \param name Unique name of plugin. On Linux platforms, it should be identical with repository and .so file names (e.g. "tcp" for finroc_plugins_tcp and libfinroc_plugins_tcp.so).
   * \param scheme_name Name of scheme that is handled by this handler
   */
  tNetworkTransportPlugin(const char* name, const char* scheme_name);

  /*!
   * \return Framework element that contains all framework elements created by plugin that are neither settings nor services
   */
  core::tFrameworkElement* GetPluginRootFrameworkElement()
  {
    return plugin_elements_root;
  }

  /*!
   * Default implementation returns any remote runtime whose name equals 'uri_authority'.
   * May be overridden to e.g. support optional default ports.
   *
   * \param uri_authority Authority part of URI
   * \return Remote runtime suitable for the provided URI authority
   */
  virtual tRemoteRuntime* GetRemoteRuntime(const std::string& uri_authority);

  /*!
   * \return Info on local runtime environment
   */
  tLocalRuntimeInfo* LocalRuntimeInfo()
  {
    return local_runtime_info.get();
  }

  /*!
   * Called whenever a new remote port is added.
   * Possibly connects port if there are any connectors that match.
   *
   * \param remote_runtime Remote runtime to which remote port belongs
   * \param port_info Info on remote port
   */
  void OnNewRemotePort(tRemoteRuntime& remote_runtime, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info);

  /*!
   * Automatically called when part is ready to start serving structure.
   * Can be used e.g. to open server port and start to actively connect
   */
  virtual void OnStartServingStructure() = 0;

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  /*!
   * \return List with representations of all remote runtimes to which there currently is an active connection. These are all initialized runtime objects.
   */
  const std::vector<tRemoteRuntime*>& ConnectedRuntimes()
  {
    return connected_runtimes;
  }

  virtual bool Create(core::tAbstractPort& owner_port, const rrlib::uri::tURI& uri, const rrlib::uri::tURIElements& uri_elements, const core::tUriConnectOptions& connect_options) override;

  virtual void Init(rrlib::xml::tNode* config_node) override;

  /*!
   * Processes incoming calls to send
   */
  void ProcessLocalRuntimeCallsToSend();

  /*!
   * Processes incoming port data changes from local runtime
   */
  void ProcessLocalRuntimePortDataChanges();

  /*!
   * Processes incoming structure changes from local runtime
   */
  void ProcessLocalRuntimeStructureChanges();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tRemoteRuntime;

  /*! Framework element that contains all framework elements created by plugin that are neither settings nor services */
  core::tFrameworkElement* plugin_elements_root;

  /*! Info on local runtime environment - created in Init() */
  std::shared_ptr<tLocalRuntimeInfo> local_runtime_info;

  /*! Copies of all URI connectors handled by this plugin (unique_ptr because connectors are referenced by tNetworkPortInfoClient) */
  std::vector<std::unique_ptr<tNetworkConnector>> network_connectors;

  /*! List with representations of all remote runtimes to which there currently is an active connection */
  std::vector<tRemoteRuntime*> connected_runtimes;


  /*!
   * Create connection for network connector object
   *
   * \param connector Network connector to create connection for
   * \param remote_runtime Runtime to which port to connect to belongs
   * \param port_info Info on port to connect to
   */
  void CreateConnection(tNetworkConnector& connector, tRemoteRuntime& remote_runtime, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info);
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
