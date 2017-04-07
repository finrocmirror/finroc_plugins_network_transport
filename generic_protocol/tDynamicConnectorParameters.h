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
/*!\file    plugins/network_transport/generic_protocol/tDynamicConnectorParameters.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-08
 *
 * \brief   Contains tDynamicConnectorParameters
 *
 * \b tDynamicConnectorParameters
 *
 * Dynamic parameters on network connector
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tDynamicConnectorParameters_h__
#define __plugins__network_transport__generic_protocol__tDynamicConnectorParameters_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

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
namespace generic_protocol
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Dynamic connector parameters
/*!
 * Dynamic parameters on network connector
 */
struct tDynamicConnectorParameters
{
  /*! Minimal time interval between updates sent (can be used to reduce bandwidth required by connectors transferring significant amounts of data) */
  rrlib::time::tDuration minimal_update_interval;

  /*! High-priority connection -> data is transferred with high priority ("express") connection if available; makes sense to set for small data in general, as bandwidth-impact should not be relevant */
  bool high_priority = false;


  /*!
   * \return Returns minimal_update_interval in Milliseconds (for legacy support)
   */
  int16_t GetMinimalUpdateIntervalInMilliseconds() const
  {
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(minimal_update_interval).count();
    return milliseconds >= std::numeric_limits<int16_t>::max() ? std::numeric_limits<int16_t>::max() : static_cast<int16_t>(milliseconds);
  }
};

inline bool operator==(const tDynamicConnectorParameters& lhs, const tDynamicConnectorParameters& rhs)
{
  return lhs.minimal_update_interval == rhs.minimal_update_interval && lhs.high_priority == rhs.high_priority;
}
inline bool operator!=(const tDynamicConnectorParameters& lhs, const tDynamicConnectorParameters& rhs)
{
  return !(lhs == rhs);
}

inline rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tDynamicConnectorParameters& data)
{
  stream << data.minimal_update_interval << data.high_priority;
  return stream;
}

inline rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tDynamicConnectorParameters& data)
{
  stream >> data.minimal_update_interval >> data.high_priority;
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
