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
/*!\file    plugins/network_transport/generic_protocol/tUriConnector.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-06
 *
 * \brief   Contains tUriConnector
 *
 * \b tUriConnector
 *
 * URI connector for connection over network transport based on this generic protocol
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tUriConnector_h__
#define __plugins__network_transport__generic_protocol__tUriConnector_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tUriConnectorData.h"
#include "plugins/network_transport/generic_protocol/tNetworkTransportPlugin.h"

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
//! Network URI connector
/*!
 * URI connector for connection over network transport based on this generic protocol
 */
class tUriConnector : public core::tUriConnector, public tUriConnectorData
{

  typedef core::tUriConnector tBase;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Note that constructor transfers ownership of this object to owner
   *
   * \param owner Reference to object that owns this connector
   * \param uri URI of partner port (preferably normalized)
   * \param parsed_uri Parsed URI of partner port
   * \param connect_options Connect options for this connector
   * \param plugin Network transport plugin that URI connector belongs to
   */
  tUriConnector(core::tAbstractPort& owner_port, const rrlib::uri::tURI& uri, const rrlib::uri::tURIElements& parsed_uri, const core::tUriConnectOptions& connect_options, tNetworkTransportPlugin& plugin);

  virtual tParameterValueRange GetParameterValues() const override
  {
    return tParameterValueRange(parameter_array.begin(), parameter_array.end());
  }

  virtual bool IsOutgoingConnector(core::tAbstractPort& owning_port) const override
  {
    return owner_port->IsOutputPort();
  }

  virtual bool SetParameter(size_t index, const rrlib::rtti::tTypedConstPointer& new_value) override;

  /*!
   * If status changes, change is published to runtime listeners
   *
   * \param new_status New status of connector.
   */
  void SetStatus(tStatus new_status)
  {
    tBase::SetStatus(new_status);
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Array with all parameter values */
  std::array<const rrlib::rtti::tTypedConstPointer, 8> parameter_array;

  /*! Local to runtime info that connector registered at */
  tLocalRuntimeInfo& local_runtime_info;


  virtual void OnDisconnect() override;

  /*!
   * Publishes event to local runtime info
   *
   * \param event Event to publish (is deleted when processed)
   */
  void PublishStructureChangeEvent(tLocalRuntimeInfo::tStructureChangeEvent* event);
};


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
