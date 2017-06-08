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
/*!\file    plugins/network_transport/runtime_info/definitions.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-26
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/definitions.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti/rtti.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tRemoteType.h"
#include "plugins/network_transport/runtime_info/tRemoteTypeConversion.h"
#include "plugins/network_transport/runtime_info/tRemoteStaticCast.h"
#include "plugins/network_transport/runtime_info/tRemoteUriSchemeHandler.h"
#include "plugins/network_transport/runtime_info/tRemoteCreateAction.h"

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

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

namespace
{
/*! Sets up published registers */
struct tSetup
{
  tSetup()
  {
    rrlib::serialization::PublishedRegisters::Register<tRemoteType, true>(rrlib::rtti::tType::GetTypeRegister(), static_cast<int>(tRegisterUIDs::TYPE));
    rrlib::serialization::PublishedRegisters::Register<tRemoteTypeConversion>(rrlib::rtti::conversion::tRegisteredConversionOperation::GetRegisteredOperations().operations, static_cast<int>(tRegisterUIDs::CONVERSION_OPERATION));
    rrlib::serialization::PublishedRegisters::Register<tRemoteStaticCast>(rrlib::rtti::conversion::tRegisteredConversionOperation::GetRegisteredOperations().static_casts, static_cast<int>(tRegisterUIDs::STATIC_CAST));
    rrlib::serialization::PublishedRegisters::Register<tRemoteUriSchemeHandler>(core::tUriConnector::GetSchemeHandlerRegister(), static_cast<int>(tRegisterUIDs::SCHEME_HANDLER));
    rrlib::serialization::PublishedRegisters::Register<tRemoteCreateAction>(runtime_construction::tCreateFrameworkElementAction::GetConstructibleElements(), static_cast<int>(tRegisterUIDs::CREATE_ACTION));
  }
} setup;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
