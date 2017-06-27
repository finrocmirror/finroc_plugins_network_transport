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
/*!\file    plugins/network_transport/generic_protocol/tUriConnectorData.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-09
 *
 * \brief   Contains tUriConnectorData
 *
 * \b tUriConnectorData
 *
 * URI connector data that is relevant for internal processing in network transport plugin.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tUriConnectorData_h__
#define __plugins__network_transport__generic_protocol__tUriConnectorData_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/definitions.h"

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
//! URI connector data
/*!
 * URI connector data that is relevant for internal processing in network transport plugin.
 */
class tUriConnectorData
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  struct tRemotePortUriComponents
  {
    /*! Normalized path of server port to connect to (no preceding slash) */
    rrlib::uri::tPath path;

    /*! Authority part of URI (server to connect to - may be empty) */
    std::string authority;
  };

  typedef tStaticConnectorParameters<tRemotePortUriComponents> tStaticParameters;

  /*!
   * \return Dynamic subscription data of this connector
   */
  const tDynamicConnectorParameters& DynamicParameters() const
  {
    return dynamic_parameters;
  }
  tDynamicConnectorParameters& DynamicParameters()
  {
    return dynamic_parameters;
  }

  /*!
   * \return Port that owns URI connector
   */
  core::tAbstractPort& OwnerPort()
  {
    return *owner_port;
  }

  /*!
   * \return Static parameters
   */
  const tStaticParameters& StaticParameters() const
  {
    return static_parameters;
  }

  /*!
   * \return (real) URI connector address for identification
   */
  const void* UriConnectorAddress() const
  {
    return uri_connector_address;
  }

//----------------------------------------------------------------------
// Protected fields and methods
//----------------------------------------------------------------------
protected:

  /*! Port that owns URI connector */
  core::tAbstractPort* owner_port = nullptr;

  /*! (real) URI connector address for identification */
  const void* uri_connector_address = nullptr;

  /*! Static parameters */
  tStaticParameters static_parameters;

  /*! Dynamic parameters */
  tDynamicConnectorParameters dynamic_parameters;
};


inline bool operator==(const tUriConnectorData::tRemotePortUriComponents& lhs, const tUriConnectorData::tRemotePortUriComponents& rhs)
{
  return lhs.path == rhs.path && lhs.authority == rhs.authority;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tUriConnectorData::tRemotePortUriComponents& data)
{
  stream << data.path << data.authority;
  return stream;
}

inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tUriConnectorData::tRemotePortUriComponents& data)
{
  stream >> data.path >> data.authority;
  return stream;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
