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
/*!\file    plugins/network_transport/generic_protocol/tNetworkConnector.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-08
 *
 * \brief   Contains tNetworkConnector
 *
 * \b tNetworkConnector
 *
 * (Virtual) connector object that connects ports in different processes over this network_transport.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tNetworkConnector_h__
#define __plugins__network_transport__generic_protocol__tNetworkConnector_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkPortInfoClient.h"

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
//! Network connector
/*!
 * Internal copy of relevant tURIConnector data
 */
class tNetworkConnector : public tUriConnectorData
{
  /*!
   * When storing port wrappers in smart pointers, can be used to to auto-delete wrapped ports using safe tPortWrapperBase::ManagedDelete()
   */
  struct tClientPortDeleter
  {
    void operator()(tNetworkPortInfoClient* ptr) const
    {
      if (ptr)
      {
        assert(ptr->NetworkPortInfo().GetAnnotated<core::tFrameworkElement>());
        ptr->NetworkPortInfo().GetAnnotated<core::tFrameworkElement>()->ManagedDelete();
      }
    }
  };

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef tStaticNetworkConnectorParameters tStaticParameters;
  typedef std::shared_ptr<tNetworkPortInfoClient> tSharedClientPort;
  typedef std::shared_ptr<data_ports::tGenericPort> tSharedTempPort;

  tNetworkConnector(const tUriConnectorData& data, const rrlib::rtti::conversion::tConversionOperationSequence& local_conversion);

  ~tNetworkConnector();

  /*!
   * \return Local conversion operations
   */
  const rrlib::rtti::conversion::tConversionOperationSequence& LocalConversionOperations() const
  {
    return local_conversion;
  }

  /*!
   * Update and publish status of original URI connector
   *
   * \param status New status
   */
  void UpdateStatus(core::tUriConnector::tStatus status) const;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tNetworkTransportPlugin;
  friend class tRemoteRuntime;
  friend class tNetworkPortInfoClient;

  /*!
   * Temporary connector port: This hidden port is created for network connector(s).
   * It has the same data type on server and client.
   */
  tSharedClientPort temporary_connector_port;

  /*! If type conversion is used, contains an additional conversion port between 'temporary_connector_port' and destination port of connection */
  tSharedTempPort temporary_conversion_port;

  /*! Local conversion operations */
  rrlib::rtti::conversion::tConversionOperationSequence local_conversion;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
