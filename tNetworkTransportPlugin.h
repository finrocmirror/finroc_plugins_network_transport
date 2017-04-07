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
/*!\file    plugins/network_transport/tNetworkTransportPlugin.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 * \brief   Contains tNetworkTransportPlugin
 *
 * \b tNetworkTransportPlugin
 *
 * Base class for all plugins that provide a network transport mechanism
 * for a whole finroc runtime environment.
 * One class in a Plugin must inherit from this interface.
 * It should be instantiated in a .cpp file.
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__tNetworkTransportPlugin_h__
#define __plugins__network_transport__tNetworkTransportPlugin_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tAbstractPort.h"
#include "plugins/parameters/tConfigurablePlugin.h"

//----------------------------------------------------------------------
// Internal includes with ""
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
// Class declaration
//----------------------------------------------------------------------
//! Base class for network transport plugins
/*!
 * Base class for all plugins that provide a network transport mechanism
 * for a whole finroc runtime environment.
 * One class in a Plugin must inherit from this interface.
 * It should be instantiated in a .cpp file.
 */
class tNetworkTransportPlugin : public parameters::tConfigurablePlugin, public core::tUriConnector::tSchemeHandler
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * \param name Unique name of plugin. On Linux platforms, it should be identical with repository and .so file names (e.g. "tcp" for finroc_plugins_tcp and libfinroc_plugins_tcp.so).
   * \param scheme_name Name of scheme that is handled by this handler
   * \param parameter_definitions Parameters that connectors of this network transport have
   */
  tNetworkTransportPlugin(const char* name, const char* scheme_name, const rrlib::rtti::tConstParameterDefinitionRange& parameter_definitions) :
    tConfigurablePlugin(name),
    tSchemeHandler(scheme_name, parameter_definitions)
  {}

  /*!
   * \return Parent element for all network transports
   */
//  static core::tFrameworkElement& GetNetworkTransportsParent();

//  Transport-specific methods can be offered in specific plugins
//  /*!
//   * Connect local port to port in remote runtime environment using this
//   * network transport plugin.
//   *
//   * \param local_port Local port to connect
//   * \param remote_port_uri Uid of port in remote runtime environment
//   * \param connect_options Options for connection
//   * \return Returns error message if connecting failed. On success an empty string is returned.
//   */
//  virtual std::string Connect(core::tAbstractPort& local_port, const rrlib::uri::tURI& remote_port_uri, const core::tUriConnectOptions& connect_options) = 0;
//
//  /*!
//   * Disconnect any connections from local port to port in remote runtime environment
//   * that were created using this transport plugin
//   *
//   * \param local_port Local port to disconnect
//   * \param remote_port_uri Uid of port in remote runtime environment
//   * \return Returns error message if disconnecting failed. On success an empty string is returned.
//   */
//  virtual std::string Disconnect(core::tAbstractPort& local_port, const rrlib::uri::tURI& remote_port_uri) = 0;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
