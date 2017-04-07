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
/*!\file    plugins/network_transport/runtime_info/definitions.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-26
 *
 * Definitions regarding serialization and representation of runtime elements for remote runtime environments.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__definitions_h__
#define __plugins__network_transport__runtime_info__definitions_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/serialization/serialization.h"

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
namespace runtime_info
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Function declarations
//----------------------------------------------------------------------

/*!
 * Register UIDs for data exchange
 */
enum class tRegisterUIDs
{
  TYPE,
  STATIC_CAST,
  CONVERSION_OPERATION,
  SCHEME_HANDLER,
  CREATE_ACTION
};

/*!
 * Enum on different levels of structure (framework elements and ports) exchanged among peers
 */
enum class tStructureExchange : int
{
  NONE,               //<! No info on structure is sent
  SHARED_PORTS,       //<! Send info on shared ports to connection partner
  COMPLETE_STRUCTURE, //<! Send info on complete structure to connection partner (e.g. for fingui)
  FINSTRUCT,          //<! Send info on complete structure including port connections to partner (as required by finstruct)
};

/*!
 * Defines custom flags used in tSerializationInfo
 */
enum tSerializationInfoFlags
{
  cSTRUCTURE_EXCHANGE_FLAG_1 = 0x1,  //!< First four flags are used to encode tStructureExchange (two bits in reserve for future extensions)
  cSTRUCTURE_EXCHANGE_FLAG_N = 0x8,
  cJAVA_CLIENT               = 0x10, //!< Connected to Java Client?
  cDEBUG_PROTOCOL            = 0x20  //!< Write debug info to protocol?
};

/*!
 * \param stream Output stream
 * \return Structure exchange level that target of stream requests
 */
inline tStructureExchange GetStructureExchangeLevel(const rrlib::serialization::tOutputStream& stream)
{
  return static_cast<tStructureExchange>(stream.GetTargetInfo().custom_info & 0xF);
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
