//
// You received this file as part of Finroc
// A Framework for intelligent robot control
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
/*!\file    plugins/network_transport/tNetworkConnection.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 * \brief   Contains tNetworkConnection
 *
 * \b tNetworkConnection
 *
 * Single network connection.
 * Encodes destination so that finstruct can identify connected port
 * in another runtime environment.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__tNetworkConnection_h__
#define __plugins__network_transport__tNetworkConnection_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/serialization/serialization.h"
#include "core/tFrameworkElement.h"

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

/*!
 * Contains possible ways of encoding port connections to elements in
 * remote runtime environments
 */
enum class tDestinationEncoding
{
  NONE,           //!< There is no destination encoded
  UUID_AND_HANDLE //!< UUID of destination runtime environment and port handle
};

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Single network connection
/*!
 * Single network connection.
 * Encodes destination so that finstruct can identify connected port
 * in another runtime environment.
 */
class tNetworkConnection
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tNetworkConnection();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnection& connection);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnection& connection);


  /*! Encoding/Identification that is used for connected element in remote runtime environment */
  tDestinationEncoding encoding;

  /*! uuid of connected runtime environment - as string */
  std::string uuid;

  /*! Handle of connected port */
  core::tFrameworkElement::tHandle port_handle;
};


rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tNetworkConnection& connection);

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tNetworkConnection& connection);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
