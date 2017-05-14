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
/*!\file    plugins/network_transport/runtime_info/tUriConnectorInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-11
 *
 * \brief   Contains tUriConnectorInfo
 *
 * \b tUriConnectorInfo
 *
 * Information on URI connectors relevant in tools and other runtime environments
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tUriConnectorInfo_h__
#define __plugins__network_transport__runtime_info__tUriConnectorInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tUriConnector.h"

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
//! URI connector info
/*!
 * Information on URI connectors relevant in tools and other runtime environments
 */
struct tUriConnectorInfo
{
  struct tID
  {
    /*! Handle of owner */
    core::tFrameworkElement::tHandle owner_handle = 0;

    /*! Index of URI connector in owner list */
    uint8_t index = 0;

    tID(const core::tUriConnector& connector) :
      owner_handle(connector.Owner().GetHandle()),
      index(GetIndex(connector))
    {}

  } id;

  struct tStaticInfo
  {
    /*! Flags and conversion operations for this connector */
    core::tConnectOptions flags_and_conversion_operations;

    /*! URI of partner port (preferably normalized) */
    rrlib::uri::tURI uri;

    /*! Scheme handler for connector's URI scheme */
    const core::tUriConnector::tSchemeHandler* scheme_handler = nullptr;

    tStaticInfo(const core::tUriConnector& connector) :
      flags_and_conversion_operations({ connector.ConversionOperations(), connector.Flags() }),
                                    uri(connector.Uri()),
                                    scheme_handler(&connector.GetSchemeHandler())
    {}

    static void Serialize(rrlib::serialization::tOutputStream& stream, const core::tConnectOptions& flags_and_conversion_operations,
                          const rrlib::uri::tURI& uri, const core::tUriConnector::tSchemeHandler& scheme_handler);

  } static_info;

  struct tDynamicInfo
  {
    /*! Status of connector */
    core::tUriConnector::tStatus status = core::tUriConnector::tStatus::DISCONNECTED;

    tDynamicInfo(const core::tUriConnector& connector) :
      status(connector.GetStatus())
    {}

  } dynamic_info;


  tUriConnectorInfo(const core::tUriConnector& connector) :
    id(connector),
    static_info(connector),
    dynamic_info(connector)
  {}

  /*!
   * \param Connector Connector whose index in owner to get of
   * \return Index in owner vector
   */
  static uint8_t GetIndex(const core::tUriConnector& connector);

  /*!
   * Serializes info on single URI connector to stream so that it can later
   * be deserialized in typically another runtime environment.
   *
   * \param stream Binary stream to serialize to
   * \param connector URI connector whose info to write to stream
   */
  static void Serialize(rrlib::serialization::tOutputStream& stream, const core::tUriConnector& connector);
};


inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tUriConnectorInfo::tID& info)
{
  stream << info.owner_handle << info.index;
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tUriConnectorInfo::tStaticInfo& info)
{
  tUriConnectorInfo::tStaticInfo::Serialize(stream, info.flags_and_conversion_operations, info.uri, *info.scheme_handler);
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tUriConnectorInfo::tDynamicInfo& info)
{
  stream << info.status;
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tUriConnectorInfo& info)
{
  stream << info.id << info.static_info << info.dynamic_info;
  return stream;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
