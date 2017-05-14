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
/*!\file    plugins/network_transport/runtime_info/tConnectorInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-11
 *
 * \brief   Contains tConnectorInfo
 *
 * \b tConnectorInfo
 *
 * Information on connectors relevant in tools and other runtime environments
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tConnectorInfo_h__
#define __plugins__network_transport__runtime_info__tConnectorInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tAbstractPort.h"

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
// Class declaration
//----------------------------------------------------------------------
//! Connector Info
/*!
 * Information on connectors relevant in tools and other runtime environments
 */
struct tConnectorInfo
{

  struct tID
  {
    /*! Handles of source and destination port */
    core::tFrameworkElement::tHandle source_handle = 0, destination_handle = 0;

    tID(const core::tConnector& connector) :
      source_handle(connector.Source().GetHandle()),
      destination_handle(connector.Destination().GetHandle())
    {}

  } id;

  friend inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tConnectorInfo::tID& info);


  struct tStaticInfo
  {
    /*! Flags and conversion operations for this connector */
    core::tConnectOptions flags_and_conversion_operations;

    tStaticInfo(const core::tConnector& connector) :
      flags_and_conversion_operations({ connector.ConversionOperations(), connector.Flags() })
    {}

  } static_info;


  tConnectorInfo(const core::tConnector& connector) :
    id(connector),
    static_info(connector)
  {}

  friend inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tID& info);
  friend inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tStaticInfo& info);

  /*!
   * Serializes info on single connector to stream so that it can later
   * be deserialized in typically another runtime environment.
   *
   * \param stream Binary stream to serialize to
   * \param connector Connector whose info to write to stream
   */
  static void Serialize(rrlib::serialization::tOutputStream& stream, const core::tConnector& connector)
  {
    stream << tID(connector);
    stream << tStaticInfo(connector);
  }
};

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tConnectorInfo::tID& info)
{
  stream << info.source_handle << info.destination_handle;
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tConnectorInfo::tStaticInfo& info)
{
  stream << info.flags_and_conversion_operations;
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tConnectorInfo& info)
{
  stream << info.id << info.static_info;
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
