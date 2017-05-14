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
/*!\file    plugins/network_transport/generic_protocol/definitions.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 * Definitions for the generic message-based protocol in this directory
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__definitions_h__
#define __plugins__network_transport__generic_protocol__definitions_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/rpc_ports/definitions.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tMessage.h"
#include "plugins/network_transport/runtime_info/tFrameworkElementInfo.h"
#include "plugins/network_transport/generic_protocol/tStaticConnectorParameters.h"
#include "plugins/network_transport/generic_protocol/tDynamicConnectionData.h"

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

/*! Protocol version */
enum { cPROTOCOL_VERSION_MAJOR = 1 };      // Major protocol version: Relevant for communication compatibility among peers
enum { cPROTOCOL_VERSION_MINOR = 1703 };   // Minor protocol version: Relevant for optional communication features among peers (may break compatibility with outdated tooling). Identical to release version number encoded in serialization info (YYMM).

/*!
 * Flags relevant for some messages (used to be tDataEncoding): Encoding of data and handles
 * (important: first three constants must be identical to rrlib::serialization::tDataEncoding)
 */
namespace message_flags
{
enum tDataEncoding
{
  cBINARY_ENCODING = 0,
  cSTRING_ENCODING = 1,
  cXML_ENCODING = 2,
  cBINARY_COMPRESSED_ENCODING = 3,
};
enum tFlags
{
  cTO_SERVER = 4,                  // message goes to server -> connection handle is not the receiver port handle
  cHIGH_PRIORITY = 8               // use high priority connection (currently only relevant for pull calls)
};
}

/*! Maximum not acknowledged Packets (64-1 (2^x for fast modulo)) */
enum { cMAX_NOT_ACKNOWLEDGED_PACKETS = 0x3F };


/*!
 * Protocol OpCodes
 */
enum class tOpCode : uint8_t
{
  // Opcodes for management connection
  SUBSCRIBE_LEGACY,    // Subscribe to data port (legacy - only supported as client)
  UNSUBSCRIBE_LEGACY,  // Unsubscribe from data port (legacy - only supported as client)
  PULLCALL,            // Pull call
  PULLCALL_RETURN,     // Returning pull call
  RPC_CALL,            // RPC call
  TYPE_UPDATE,         // Update on remote type info (typically desired update time)
  STRUCTURE_CREATED,   // Update on remote framework elements: Element created
  STRUCTURE_CHANGED,   // Update on remote framework elements: Port changed
  STRUCTURE_DELETED,   // Update on remote framework elements: Element deleted
  PEER_INFO,           // Information about other peers

  // Change event opcodes (from subscription - or for plain setting of port)
  PORT_VALUE_CHANGE,                         // normal variant
  SMALL_PORT_VALUE_CHANGE,                   // variant with max. 256 byte message length (3 bytes smaller than normal variant)
  SMALL_PORT_VALUE_CHANGE_WITHOUT_TIMESTAMP, // variant with max. 256 byte message length and no timestamp (11 bytes smaller than normal variant)

  // new commands in 17.03
  CONNECT_PORTS,         // Connect ports in different processes
  CONNECT_PORTS_ERROR,   // Notifies client on connection error
  UPDATE_CONNECTION,     // Change dynamic connection data of connection created with 'CONNECT_PORTS'
  DISCONNECT_PORTS,      // Disconnect ports connected with 'CONNECT_PORT'
  CONNECTOR_CREATED,     // Connector created/added
  CONNECTOR_DELETED,     // Connector deleted
  URI_CONNECTOR_CREATED, // URI Connector created/added
  URI_CONNECTOR_UPDATED, // URI Connector changed/updated
  URI_CONNECTOR_DELETED, // URI Connector deleted

  // Used for custom additional messages - possibly without opcode
  OTHER
};

typedef rpc_ports::internal::tCallId tCallId;
typedef typename core::tFrameworkElement::tHandle tFrameworkElementHandle;
typedef tStaticConnectorParameters<tFrameworkElementHandle> tStaticNetworkConnectorParameters;

/*!
 * Helper to read message size from stream
 * Exists once for each opcode
 */
struct tMessageSizeReader
{
  tMessageSize message_size;
  size_t argument_size;

  size_t ReadMessageSize(rrlib::serialization::tInputStream& stream) const
  {
    if (message_size == tMessageSize::FIXED)
    {
      return argument_size + ((stream.GetSourceInfo().custom_info & runtime_info::cDEBUG_PROTOCOL) ? 1 : 0);
    }
    else if (message_size == tMessageSize::VARIABLE_UP_TO_4GB)
    {
      return stream.ReadInt();
    }
    else
    {
      return stream.ReadNumber<uint8_t>();
    }
  }
};

/*!
 * Lookup for message size reader for each opcode
 */
extern const tMessageSizeReader message_size_for_opcodes[static_cast<int>(tOpCode::OTHER)];

////////////////////
// Message types
////////////////////

// Parameters: [server port handle][int16: strategy][bool: reverse push][int16: update interval][client port handle][desired encoding]
typedef tMessage < tOpCode, tOpCode::SUBSCRIBE_LEGACY, tMessageSize::FIXED, tFrameworkElementHandle, int16_t, bool,
        int16_t, tFrameworkElementHandle, uint8_t > tLegacySubscribeMessage;

// Parameters: [server port handle]
typedef tMessage<tOpCode, tOpCode::UNSUBSCRIBE_LEGACY, tMessageSize::FIXED, tFrameworkElementHandle> tLegacyUnsubscribeMessage;

// Parameters: [int32: acknowledged message batch]
//typedef tMessage<tOpCode::ACK, tMessageSize::FIXED, int32_t> tAckMessage;

// Parameters: [connection handle == client port handle (legacy: remote port handle)][call uid][desired encoding]
typedef tMessage<tOpCode, tOpCode::PULLCALL, tMessageSize::FIXED, tFrameworkElementHandle, tCallId, uint8_t> tPullCall;

// Parameters: [call uid][failed?] after message: [type][timestamp][serialized data]
typedef tMessage<tOpCode, tOpCode::PULLCALL_RETURN, tMessageSize::VARIABLE_UP_TO_4GB, tCallId, bool> tPullCallReturn;

// Parameters: [sender remote port handle][call type] after message: [pass stream to DeserializeCallFunction of tMessage]
typedef tMessage<tOpCode, tOpCode::RPC_CALL, tMessageSize::VARIABLE_UP_TO_4GB, tFrameworkElementHandle, rpc_ports::tCallType> tRPCCall;

// Parameters: after message: [data type][int16: new update time]
typedef tMessage<tOpCode, tOpCode::TYPE_UPDATE, tMessageSize::VARIABLE_UP_TO_4GB> tTypeUpdateMessage;

// Parameters: [framework element handle] after message: [tFrameworkElementInfo::tStaticInfo][tFrameworkElementInfo::tDynamicInfo]
typedef tMessage<tOpCode, tOpCode::STRUCTURE_CREATED, tMessageSize::VARIABLE_UP_TO_4GB, tFrameworkElementHandle> tStructureCreatedMessage;

// Parameters: [framework element handle] after message: [tFrameworkElementInfo::tDynamicInfo]  (variable size is kept for backward-compatibility)
typedef tMessage<tOpCode, tOpCode::STRUCTURE_CHANGED, tMessageSize::VARIABLE_UP_TO_4GB, tFrameworkElementHandle> tStructureChangedMessage;

// Parameters: [framework element handle]
typedef tMessage<tOpCode, tOpCode::STRUCTURE_DELETED, tMessageSize::FIXED, tFrameworkElementHandle> tStructureDeletedMessage;

// Parameters: after message: [list of [tPeerInfo.uuid, tPeerInfo.peer_type, tPeerInfo.name, tPeerInfo.addresses]]
// Transport-specific opcode; message type in generic protocol to preserve backward-compatability; not used/handled in generic protocol
typedef tMessage<tOpCode, tOpCode::PEER_INFO, tMessageSize::VARIABLE_UP_TO_4GB> tPeerInfoMessage;


// Parameters: [connection handle == client port handle (legacy: remote port handle)][encoding] after message: [binary blob or null-terminated string depending on type encoding]
typedef tMessage <tOpCode, tOpCode::PORT_VALUE_CHANGE, tMessageSize::VARIABLE_UP_TO_4GB, int32_t, uint8_t> tPortValueChange;

// Parameters: [connection handle == client port handle (legacy: remote port handle)][encoding] after message: [binary blob or null-terminated string depending on type encoding]
typedef tMessage <tOpCode, tOpCode::SMALL_PORT_VALUE_CHANGE, tMessageSize::VARIABLE_UP_TO_255_BYTE, int32_t, uint8_t> tSmallPortValueChange;

// Parameters: [connection handle == client port handle (legacy: remote port handle)][encoding] after message: [binary blob or null-terminated string depending on type encoding]
typedef tMessage < tOpCode, tOpCode::SMALL_PORT_VALUE_CHANGE_WITHOUT_TIMESTAMP, tMessageSize::VARIABLE_UP_TO_255_BYTE, int32_t,
        uint8_t > tSmallPortValueChangeWithoutTimestamp;


// Parameters: [connection handle == client port handle] after message: [static connector parameters][dynamic connection data]
typedef tMessage <tOpCode, tOpCode::CONNECT_PORTS, tMessageSize::VARIABLE_UP_TO_4GB, tFrameworkElementHandle> tConnectPortsMessage;

// Parameters: [connection handle == client port handle] after message: [string: error message]
typedef tMessage <tOpCode, tOpCode::CONNECT_PORTS_ERROR, tMessageSize::VARIABLE_UP_TO_4GB, tFrameworkElementHandle> tConnectPortsErrorMessage;

// Parameters: [connection handle == client port handle][duration][high_priority][strategy]
typedef tMessage <tOpCode, tOpCode::UPDATE_CONNECTION, tMessageSize::FIXED, tFrameworkElementHandle, rrlib::time::tDuration, bool, int16_t> tUpdateConnectionMessage;

// Parameters: [connection handle == client port handle]
typedef tMessage <tOpCode, tOpCode::DISCONNECT_PORTS, tMessageSize::FIXED, tFrameworkElementHandle> tDisconnectPortsMessage;

// Parameters: after message: [tConnectorInfo]
typedef tMessage<tOpCode, tOpCode::CONNECTOR_CREATED, tMessageSize::VARIABLE_UP_TO_4GB> tConnectorCreatedMessage;

// Parameters: [source port handle][server destination port handle]
typedef tMessage<tOpCode, tOpCode::CONNECTOR_DELETED, tMessageSize::FIXED, tFrameworkElementHandle, tFrameworkElementHandle> tConnectorDeletedMessage;

// Parameters: after message: [tURIConnectorInfo]
typedef tMessage<tOpCode, tOpCode::URI_CONNECTOR_CREATED, tMessageSize::VARIABLE_UP_TO_4GB> tUriConnectorCreatedMessage;

// Parameters: [owner port handle][URI connector index][connector status]
typedef tMessage<tOpCode, tOpCode::URI_CONNECTOR_UPDATED, tMessageSize::FIXED, tFrameworkElementHandle, uint8_t, core::tUriConnector::tStatus> tUriConnectorUpdatedMessage;

// Parameters: [owner port handle][URI connector index]
typedef tMessage<tOpCode, tOpCode::URI_CONNECTOR_DELETED, tMessageSize::FIXED, tFrameworkElementHandle, uint8_t> tUriConnectorDeletedMessage;

//----------------------------------------------------------------------
// Function declarations
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
