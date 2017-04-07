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
/*!\file    plugins/network_transport/generic_protocol/definitions.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/definitions.h"

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

template <typename T>
constexpr tMessageSizeReader CreateReader()
{
  return tMessageSizeReader { T::MessageSize(), T::cARGUMENT_SIZE };
}

const tMessageSizeReader message_size_for_opcodes[static_cast<int>(tOpCode::OTHER)] =
{
  CreateReader<tLegacySubscribeMessage>(),
  CreateReader<tLegacyUnsubscribeMessage>(),
  CreateReader<tPullCall>(),
  CreateReader<tPullCallReturn>(),
  CreateReader<tRPCCall>(),
  CreateReader<tTypeUpdateMessage>(),
  CreateReader<tStructureCreatedMessage>(),
  CreateReader<tStructureChangedMessage>(),
  CreateReader<tStructureDeletedMessage>(),
  CreateReader<tPeerInfoMessage>(),
  CreateReader<tPortValueChange>(),
  CreateReader<tSmallPortValueChange>(),
  CreateReader<tSmallPortValueChangeWithoutTimestamp>(),
  CreateReader<tConnectPortsMessage>(),
  CreateReader<tConnectPortsErrorMessage>(),
  CreateReader<tUpdateConnectionMessage>(),
  CreateReader<tDisconnectPortsMessage>(),
  CreateReader<tConnectorCreatedMessage>(),
  CreateReader<tConnectorDeletedMessage>(),
  CreateReader<tUriConnectorCreatedMessage>(),
  CreateReader<tUriConnectorUpdatedMessage>(),
  CreateReader<tUriConnectorDeletedMessage>(),
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
