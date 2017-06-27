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
/*!\file    plugins/network_transport/runtime_info/tFrameworkElementInfo.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-01-12
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tFrameworkElementInfo.h"

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
typedef core::tFrameworkElement::tFlag tFlag;

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

namespace internal
{
void Serialize(rrlib::serialization::tOutputStream& stream, const core::tFrameworkElement::tFlags& flags, const tLinkArray& link_data,
               const std::array<const std::string*, cMAX_LINKS>* alt_link_name_storage, uint8_t link_count, const rrlib::rtti::tType& type, const std::vector<std::string>& tags)
{
  tStructureExchange structure_exchange_level = static_cast<tStructureExchange>(stream.GetTargetInfo().custom_info & 0xF);
  if (structure_exchange_level == tStructureExchange::NONE)
  {
    FINROC_LOG_PRINT_STATIC(WARNING, "Specifying structure exchange level tStructureExchange::NONE does not write anything to stream. This is typically not intended.");
    return;
  }
  if (stream.GetTargetInfo().revision == 0)
  {
    FINROC_LOG_PRINT_STATIC(WARNING, "No structure info should be sent to old parts, as serialization changed.");
  }
  if (link_count > cMAX_LINKS)
  {
    FINROC_LOG_PRINT(WARNING, "Only three links are supported");
    link_count = cMAX_LINKS;
  }

  if (stream.GetTargetInfo().custom_info & cJAVA_CLIENT)
  {
    stream << flags;
    stream << link_count;
    for (size_t i = 0; i < link_count; i++)
    {
      if (structure_exchange_level == tStructureExchange::SHARED_PORTS)
      {
        assert(link_data[i].path != rrlib::uri::tPath());
        stream << link_data[i].path;
      }
      else
      {
        if (alt_link_name_storage)
        {
          stream << (*(*alt_link_name_storage)[i]);
        }
        else
        {
          stream << link_data[i].name;
        }
        stream << link_data[i].parent_handle;
      }
    }

    if (flags.Get(core::tFrameworkElementFlag::PORT))
    {
      stream << type;
    }

    if (structure_exchange_level == tStructureExchange::FINSTRUCT)
    {
      stream.WriteBoolean(tags.size());
      if (tags.size())
      {
        stream << tags;
      }
    }
  }
  else
  {
    assert(structure_exchange_level == tStructureExchange::SHARED_PORTS && flags.Get(core::tFrameworkElementFlag::PORT));

    stream << link_count;
    for (size_t i = 0; i < link_count; i++)
    {
      stream << link_data[i].path;
    }
    stream << type << flags;
  }
}

}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tRemoteFrameworkElementInfo& info)
{
  info.static_info.link_count = stream.ReadByte();
  if (info.static_info.link_count > info.static_info.link_data.size())
  {
    throw std::runtime_error("More than 3 paths received");
  }

  for (int i = 0; i < info.static_info.link_count; i++)
  {
    if (stream.GetSourceInfo().revision == 0)
    {
      info.static_info.link_data[i].path = rrlib::uri::tPath(stream.ReadString());
      stream.ReadBoolean(); // unique link? (obsolete)
    }
    else
    {
      stream >> info.static_info.link_data[i].path;
    }
  }

  // We only receive (shared) ports
  info.static_info.type = &stream.ReadRegisterEntry<tRemoteType>();
  stream >> info.static_info.flags;
  if ((info.static_info.type->GetTypeTraits() & rrlib::rtti::trait_flags::cIS_DATA_TYPE) || stream.GetSourceInfo().revision == 0)
  {
    stream >> info.dynamic_info;
  }

  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
