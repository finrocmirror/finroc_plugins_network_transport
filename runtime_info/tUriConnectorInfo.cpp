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
/*!\file    plugins/network_transport/runtime_info/tUriConnectorInfo.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-11
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tUriConnectorInfo.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

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
namespace runtime_info
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

uint8_t tUriConnectorInfo::GetIndex(const core::tUriConnector& connector)
{
  auto& connectors = connector.Owner().UriConnectors();
  for (auto it = connectors.begin(); it != connectors.end(); ++it)
  {
    if (it->get() == &connector)
    {
      return static_cast<uint8_t>(it - connectors.begin());
    }
  }
  throw rrlib::util::tTraceableException<std::invalid_argument>("Connector is not in list of owner (programming error)");
}

void tUriConnectorInfo::Serialize(rrlib::serialization::tOutputStream& stream, const core::tUriConnector& connector)
{
  stream << tID(connector);
  core::tConnectOptions options = { connector.ConversionOperations(), connector.Flags() };
  tStaticInfo::Serialize(stream, options, connector.Uri(), connector.GetSchemeHandler());
  stream << tDynamicInfo(connector);
}

void tUriConnectorInfo::tStaticInfo::Serialize(rrlib::serialization::tOutputStream& stream, const core::tConnectOptions& flags_and_conversion_operations, const rrlib::uri::tURI& uri, const core::tUriConnector::tSchemeHandler& scheme_handler)
{
  flags_and_conversion_operations.Serialize(stream, false);
  stream << uri << scheme_handler;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
