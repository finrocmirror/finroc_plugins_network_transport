//
// You received this file as part of Finroc
// A framework for intelligent robot control
//
// Copyright (C) AG Robotersysteme TU Kaiserslautern
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
/*!\file    plugins/network_transport/runtime_info/tRemoteRuntime.h
 *
 * \author  Max Reichardt
 *
 * \date    2014-09-19
 *
 * \brief   Contains tRemoteRuntime
 *
 * \b tRemoteRuntime
 *
 * Base class for remote runtime environments that finstruct cannot access directly
 * (e.g. running on embedded hardware).
 * This class provides access for finstruct, manages structure information and tunnels requests.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tRemoteRuntime_h__
#define __plugins__network_transport__runtime_info__tRemoteRuntime_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/tOutputPort.h"

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
//! Provides access to remote runtime environments
/*!
 * Base class for remote runtime environments that finstruct cannot access directly
 * (e.g. running on embedded hardware).
 * This class provides access for finstruct, manages structure information and tunnels requests.
 */
class tRemoteRuntime : public core::tFrameworkElement
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Port that publishes updates on remote runtime's structure.
   * On new connections, the whole structure is transferred
   * (this is done via the same port in order to synchronize on updates)
   */
  data_ports::tOutputPort<rrlib::serialization::tMemoryBuffer> structure_updates_port;


  /*!
   * \param protocol Id of protocol used to access this node
   * (other parameters: see tFrameworkElement constructor)
   */
  tRemoteRuntime(const std::string& protocol, tFrameworkElement* parent = NULL, const tString& name = "", tFlags flags = tFlags());

  /*!
   * Creates structure update port - which will initially serve the structure passed to this function.
   *
   * \param current_runtime_info Structure information that will be served initially
   */
  void InitRemoteStructure(const rrlib::serialization::tFixedBuffer& current_runtime_info);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:



};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
