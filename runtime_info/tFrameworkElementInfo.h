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
/*!\file    plugins/network_transport/runtime_info/tFrameworkElementInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-01-12
 *
 * \brief   Contains tFrameworkElementInfo
 *
 * \b tFrameworkElementInfo
 *
 * Information on (shared) framework elements - as exchanged by peers.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tFrameworkElementInfo_h__
#define __plugins__network_transport__runtime_info__tFrameworkElementInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/common/tAbstractDataPort.h"
#include "plugins/data_ports/type_traits.h"
#include "core/tFrameworkElementTags.h"
#include "plugins/data_ports/common/tAbstractDataPort.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/definitions.h"
#include "plugins/network_transport/runtime_info/tConnectorInfo.h"
#include "plugins/network_transport/runtime_info/tRemoteType.h"
#include "plugins/network_transport/runtime_info/tUriConnectorInfo.h"

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

namespace internal
{

/*! Maximum number of links per port */
enum { cMAX_LINKS = 3 };

struct tLink
{
  /*! Path to this port (only required for shared ports) */
  rrlib::uri::tPath path;

  /*! Name of this framework element (from parent) */
  std::string name;

  /*! Handle of parent */
  core::tFrameworkElement::tHandle parent_handle = 0;
};

typedef std::array<tLink, cMAX_LINKS> tLinkArray;

void Serialize(rrlib::serialization::tOutputStream& stream, const core::tFrameworkElement::tFlags& flags, const tLinkArray& link_data,
               const std::array<const std::string*, cMAX_LINKS>* alt_link_name_storage, uint8_t link_count, const rrlib::rtti::tType& type, const std::vector<std::string>& tags);
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Framework element information
/*!
 * Information on (shared) framework elements - as exchanged by peers.
 *
 * In C++, this struct can only store info shared ports.
 * The static Serialize method also serializes framework element info
 * and port connection info.
 */
template <bool Tremote>
struct tFrameworkElementInfo
{
  typedef typename core::tFrameworkElement::tHandle tHandle;
  typedef typename core::tFrameworkElement::tFlag tFlag;
  typedef typename core::tFrameworkElement::tFlags tFlags;
  typedef typename std::conditional<Tremote, const tRemoteType*, rrlib::rtti::tType>::type tType;

  struct tID
  {
    /*! Handle of element in runtime environment */
    core::tFrameworkElement::tHandle handle = 0;

    tID() : handle(0)
    {}
    tID(const core::tFrameworkElement& element) :
      handle(element.GetHandle())
    {}

  } id;

  struct tStaticInfo
  {
    /*! Framework element flags */
    core::tFrameworkElement::tFlags flags;

    typedef internal::tLinkArray tLinkArray;

    /*! Per-link data - in fixed array for efficiency reasons */
    tLinkArray link_data;

    /*! Number of links */
    uint8_t link_count;

    /*! Type of port data */
    tType type;

    /*! Classification tags (strings) assigned to framework element */
    std::vector<std::string> tags;


    tStaticInfo() : link_count(0)
    {}

    template < bool ENABLE = !Tremote >
    tStaticInfo(typename std::enable_if<ENABLE, const core::tFrameworkElement>::type& element) :
      flags(element.GetAllFlags()),
      link_count(0),
      tags(core::tFrameworkElementTags::GetTags(element))
    {
      link_count = element.IsPort() ? element.GetLinkCount() : 1;
      if (link_count > link_data.size())
      {
        FINROC_LOG_PRINT(WARNING, "Only three links are supported");
        link_count = link_data.size();
      }

      bool shared_port = IsSharedPort(element);
      for (size_t i = 0; i < link_count; i++)
      {
        if (shared_port)
        {
          element.GetPath(link_data[i].path, i);
          link_data[i].path = link_data[i].path.MakeRelative();
        }
        link_data[i].name = element.GetLink(i).GetName();
        link_data[i].parent_handle = element.GetParent(i)->GetHandle();
      }

      if (element.IsPort())
      {
        type = static_cast<const core::tAbstractPort&>(element).GetDataType();
      }
    }

    template < bool ENABLE = !Tremote >
    typename std::enable_if<ENABLE, bool>::type IsDataPort() const
    {
      return flags.Get(core::tFrameworkElementFlag::PORT) && data_ports::IsDataFlowType(type);
    }

  } static_info;

  /*! Part of information that may be changed later */
  struct tDynamicInfo
  {
    /*! Only relevant for ports: Strategy to use for port if it is destination port */
    int16_t strategy;

    tDynamicInfo() : strategy(-1)
    {}
    tDynamicInfo(const core::tFrameworkElement& element) :
      strategy(-1)
    {
      if (IsDataPort(element))
      {
        strategy = static_cast<const data_ports::common::tAbstractDataPort&>(element).GetStrategy();
      }
    }

  } dynamic_info;


  tFrameworkElementInfo() : id(), static_info(), dynamic_info()
  {}

  template < bool ENABLE = !Tremote >
  tFrameworkElementInfo(typename std::enable_if<ENABLE, const core::tFrameworkElement>::type& element) : id(element), static_info(element), dynamic_info(element)
  {}

  /*!
   * \return Whether provided element is a data flow port
   */
  static bool IsDataPort(const core::tFrameworkElement& framework_element)
  {
    return framework_element.IsPort() && data_ports::IsDataFlowType(static_cast<const core::tAbstractPort&>(framework_element).GetDataType());
  }

  /*!
   * \return Whether provided element a shared port (to be announced to other peers)?
   */
  static bool IsSharedPort(const core::tFrameworkElement& framework_element)
  {
    return framework_element.IsPort() && framework_element.GetFlag(core::tFrameworkElement::tFlag::SHARED) &&
           (!framework_element.GetFlag(core::tFrameworkElement::tFlag::NETWORK_ELEMENT));
  }

  /*!
   * Serializes info one single framework element to stream so that it can later
   * be deserialized in typically another runtime environment.
   * Note that its handle is also serialized.
   *
   * \param stream Binary stream to serialize to
   * \param framework_element Framework element to serialize info of
   * \param serialize_owned_connectors Whether to serialize connectors owned by framework element
   *
   * TODO: avoid frequent reallocation of (typically one) tPath
   */
  template < bool ENABLE = !Tremote >
  static void Serialize(typename std::enable_if<ENABLE, rrlib::serialization::tOutputStream>::type& stream, const core::tFrameworkElement& framework_element, bool serialize_owned_connectors)
  {
    rrlib::uri::tURI uri;

    // collect links
    internal::tLinkArray link_data;
    std::array<const std::string*, internal::cMAX_LINKS> alt_link_name_storage;
    size_t link_count = framework_element.GetLinkCount();
    if (link_count > link_data.size())
    {
      FINROC_LOG_PRINT(WARNING, "Only three links are supported");
      link_count = link_data.size();
    }

    tStaticInfo static_info;
    bool shared_port = IsSharedPort(framework_element);
    for (size_t i = 0; i < link_count; i++)
    {
      if (shared_port)
      {
        framework_element.GetPath(link_data[i].path, i);
        link_data[i].path = link_data[i].path.MakeRelative();
      }
      alt_link_name_storage[i] = &framework_element.GetName();
      link_data[i].parent_handle = framework_element.GetParent(i)->GetHandle();
    }

    // Serialize
    stream << tID(framework_element);
    internal::Serialize(stream, framework_element.GetAllFlags(), link_data, &alt_link_name_storage, link_count, framework_element.IsPort() ? static_cast<const core::tAbstractPort&>(framework_element).GetDataType() : rrlib::rtti::tType(), core::tFrameworkElementTags::GetTags(framework_element));
    if (IsDataPort(framework_element))
    {
      stream << tDynamicInfo(framework_element);
    }

    enum tNextConnector { NONE, PLAIN, URI };

    if (serialize_owned_connectors && framework_element.IsPort())
    {
      const core::tAbstractPort& port = static_cast<const core::tAbstractPort&>(framework_element);
      for (auto it = port.OutgoingConnectionsBegin(); it != port.OutgoingConnectionsEnd(); ++it)
      {
        runtime_info::tConnectorInfo info(*it);
        stream.WriteByte(static_cast<uint8_t>(PLAIN));
        stream << it->Destination().GetHandle() << info.static_info;
      }
      for (auto & connector : port.UriConnectors())
      {
        if (connector)
        {
          tUriConnectorInfo info(*connector);
          stream.WriteByte(static_cast<uint8_t>(URI));
          stream << info.id.index << info.static_info << info.dynamic_info;
        }
      }
      stream.WriteByte(static_cast<uint8_t>(NONE));
    }
  }

};

typedef tFrameworkElementInfo<false> tLocalFrameworkElementInfo;
typedef tFrameworkElementInfo<true> tRemoteFrameworkElementInfo;

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tRemoteFrameworkElementInfo& info);

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tLocalFrameworkElementInfo::tID& info)
{
  stream << info.handle;
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tLocalFrameworkElementInfo::tStaticInfo& info)
{
  internal::Serialize(stream, info.flags, info.link_data, nullptr, info.link_count, info.type, info.tags);
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tLocalFrameworkElementInfo::tDynamicInfo& info)
{
  stream << info.strategy;
  // As structure is only sent to client with new protocol, we can omit network update time
  return stream;
}

inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tRemoteFrameworkElementInfo::tDynamicInfo& info)
{
  if (stream.GetSourceInfo().revision == 0)
  {
    stream >> info.strategy;
    stream.ReadShort();
  }
  else
  {
    stream >> info.strategy;
  }
  return stream;
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tLocalFrameworkElementInfo& info)
{
  stream << info.id << info.static_info;
  if (info.static_info.IsDataPort())
  {
    stream << info.dynamic_info;;
  }
  return stream;
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}

#endif
