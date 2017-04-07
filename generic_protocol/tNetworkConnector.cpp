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
/*!\file    plugins/network_transport/generic_protocol/tNetworkConnector.cpp
 *
 * \author  Max Reicahrdt
 *
 * \date    2017-03-08
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tNetworkConnector.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tUriConnector.h"

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
namespace generic_protocol
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

//----------------------------------------------------------------------
// tNetworkConnector constructors
//----------------------------------------------------------------------
tNetworkConnector::tNetworkConnector(const tUriConnectorData& data, const rrlib::rtti::conversion::tConversionOperationSequence& local_conversion) :
  tUriConnectorData(data),
  local_conversion(local_conversion)
{}

//----------------------------------------------------------------------
// tNetworkConnector destructor
//----------------------------------------------------------------------
tNetworkConnector::~tNetworkConnector()
{
  if (!temporary_connector_port)
  {
    return;
  }
  core::tAbstractPort* owner_port = temporary_connector_port->GetPort();
  if (!owner_port)
  {
    return;
  }
  temporary_connector_port->GetPort()->DisconnectFrom(*owner_port);
  if (temporary_conversion_port)
  {
    temporary_conversion_port->GetWrapped()->DisconnectFrom(*owner_port);
  }

  for (auto it = temporary_connector_port->used_by_connectors.begin(); it != temporary_connector_port->used_by_connectors.end(); ++it)
  {
    if ((*it) == this)
    {
      temporary_connector_port->used_by_connectors.erase(it);
      return;
    }
  }
  FINROC_LOG_PRINT(WARNING, "Connector not in vector (programming error)");
}

void tNetworkConnector::UpdateStatus(core::tUriConnector::tStatus status) const
{
  if ((!core::tRuntimeEnvironment::ShuttingDown()) && owner_port && owner_port->IsReady())
  {
    for (auto & connector_pointer : owner_port->UriConnectors())
    {
      if (connector_pointer.get() == uri_connector_address)
      {
        tUriConnector& connector = static_cast<tUriConnector&>(*connector_pointer);
        connector.SetStatus(status);
      }
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
