//
// You received this file as part of Finroc
// A Framework for intelligent robot control
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
/*!\file    plugins/network_transport/tNetworkTransportPlugin.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-11-28
 *
 * \brief   Contains tNetworkTransportPlugin
 *
 * \b tNetworkTransportPlugin
 *
 * Base class for all plugins that provide a network transport mechanism
 * for a whole finroc runtime environment.
 * One class in a Plugin must inherit from this interface.
 * It should be instantiated in a .cpp file.
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__tNetworkTransportPlugin_h__
#define __plugins__network_transport__tNetworkTransportPlugin_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tPlugin.h"

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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Base class for network transport plugins
/*!
 * Base class for all plugins that provide a network transport mechanism
 * for a whole finroc runtime environment.
 * One class in a Plugin must inherit from this interface.
 * It should be instantiated in a .cpp file.
 */
class tNetworkTransportPlugin : public core::tPlugin
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tNetworkTransportPlugin();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*!
   * \return Returns a list of all network transport plugins that have been registered for current finroc runtime environment
   */
  static const std::vector<tNetworkTransportPlugin*>& GetAll();

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
