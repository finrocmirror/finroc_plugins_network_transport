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
/*!\file    plugins/network_transport/runtime_info/tRemoteCreateAction.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-27
 *
 * \brief   Contains tRemoteCreateAction
 *
 * \b tRemoteCreateAction
 *
 * Represents tCreateFrameworkElementAction in remote runtime environment.
 *
 * Remote create actions are currently only used in Java tools.
 * Therefore, only serialization is implemented
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tRemoteCreateAction_h__
#define __plugins__network_transport__runtime_info__tRemoteCreateAction_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tCreateFrameworkElementAction.h"

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
//! Remote create action
/*!
 * Represents tCreateFrameworkElementAction in remote runtime environment.
 *
 * Remote create actions are currently only used in Java tools.
 * Therefore, only serialization is implemented
 */
class tRemoteCreateAction : public rrlib::serialization::PublishedRegisters::tRemoteEntryBase<uint32_t, runtime_construction::tCreateFrameworkElementAction::tRegister>
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  void DeserializeRegisterEntry(rrlib::serialization::tInputStream& stream)
  {}

  static void SerializeRegisterEntry(rrlib::serialization::tOutputStream& stream, const runtime_construction::tCreateFrameworkElementAction* create_action)
  {
    stream.WriteString(create_action->GetName());
    stream.WriteString(create_action->GetModuleGroup().ToString());
    stream.WriteBoolean(create_action->IsDeprecated());
    stream.WriteBoolean(create_action->GetParameterTypes());
    if (create_action->GetParameterTypes())
    {
      stream << *create_action->GetParameterTypes();
    }
  }

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
