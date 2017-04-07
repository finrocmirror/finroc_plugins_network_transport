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
/*!\file    plugins/network_transport/generic_protocol/tNetworkPortInfoClient.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-12
 *
 * \brief   Contains tNetworkPortInfoClient
 *
 * \b tNetworkPortInfoClient
 *
 * Extended network port info for data ports on client side
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tNetworkPortInfoClient_h__
#define __plugins__network_transport__generic_protocol__tNetworkPortInfoClient_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfo.h"

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
class tNetworkConnector;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Client-only network port info
/*!
 * Extended network port info for data ports on client side
 */
class tNetworkPortInfoClient
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * \param Reference to tNetworkPortInfo that this object is attached to
   */
  tNetworkPortInfoClient(tNetworkPortInfo& network_port_info, std::pair<const tFrameworkElementHandle, tRemoteRuntime::tRemotePortInfo>& port_info);

  ~tNetworkPortInfoClient();


  /*!
   * Performs subscription check
   */
  void CheckSubscription();

  /*!
   * \return Annotated client port
   */
  core::tAbstractPort* GetPort()
  {
    return NetworkPortInfo().GetAnnotated<core::tAbstractPort>();
  }

  /*!
   * \return Reference to tNetworkPortInfo that this object is attached to
   */
  tNetworkPortInfo& NetworkPortInfo()
  {
    return network_port_info;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tNetworkConnector;
  friend class tNetworkTransportPlugin;
  friend class tRemoteRuntime;
  friend class tNetworkPortInfo;

  /*! Reference to tNetworkPortInfo that this object is attached to */
  tNetworkPortInfo& network_port_info;

  /*! Is port connected? True after initial call to CheckSubscription */
  bool connected;

  /*! Connectors that use this client port */
  std::vector<tNetworkConnector*> used_by_connectors;


  void OnManagedDelete();
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
