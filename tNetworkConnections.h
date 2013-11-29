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
/*!\file    plugins/network_transport/tNetworkConnections.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 * \brief   Contains tNetworkConnections
 *
 * \b tNetworkConnections
 *
 * Annotation that manages and bundles all network connection that a port has.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__tNetworkConnections_h__
#define __plugins__network_transport__tNetworkConnections_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/tNetworkConnection.h"

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
//! Network connections of a port
/*!
 * Annotation that manages and bundles all network connection that a port has.
 */
class tNetworkConnections : public core::tAnnotation
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tNetworkConnections();

  /*!
   * \return Number of network connections stored in this annotation
   */
  size_t Count()
  {
    return connections.size();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnections& connections);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnections& connections);

  /** Network connections of port */
  std::vector<tNetworkConnection> connections;

};

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnections& connections);

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnections& connections);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
