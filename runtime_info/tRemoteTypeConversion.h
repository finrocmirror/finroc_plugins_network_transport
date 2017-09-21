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
/*!\file    plugins/network_transport/runtime_info/tRemoteTypeConversion.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-27
 *
 * \brief   Contains tRemoteTypeConversion
 *
 * \b tRemoteTypeConversion
 *
 * Represents type conversion operation from rrlib_rtti_conversion in remote runtime.
 *
 * Remote type conversions are currently only used in Java tools.
 * Therefore, only serialization is implemented
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tRemoteTypeConversion_h__
#define __plugins__network_transport__runtime_info__tRemoteTypeConversion_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti_conversion/tRegisteredConversionOperation.h"

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
//! Remote type conversion operation
/*!
 * Represents type conversion operation from rrlib_rtti_conversion in remote runtime.
 *
 * Remote type conversions are currently only used in Java tools.
 * Therefore, only serialization is implemented
 */
class tRemoteTypeConversion : public rrlib::serialization::PublishedRegisters::tRemoteEntryBase<uint16_t, rrlib::rtti::conversion::tRegisteredConversionOperation::tRegisteredOperations::tOperationsRegister>
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  void DeserializeRegisterEntry(rrlib::serialization::tInputStream& stream)
  {}

  static void SerializeRegisterEntry(rrlib::serialization::tOutputStream& stream, const rrlib::rtti::conversion::tRegisteredConversionOperation* operation)
  {
    stream << operation->Name() << operation->SupportedSourceTypes().filter;
    if (operation->SupportedSourceTypes().filter == rrlib::rtti::conversion::tSupportedTypeFilter::SINGLE)
    {
      stream << operation->SupportedSourceTypes().single_type;
    }
    stream << operation->SupportedDestinationTypes().filter;
    if (operation->SupportedDestinationTypes().filter == rrlib::rtti::conversion::tSupportedTypeFilter::SINGLE)
    {
      stream << operation->SupportedDestinationTypes().single_type;
    }

    enum { HAS_PARAMETER = 1, NOT_USUALLY_COMBINED_WITH = 2 };
    uint8_t flags = (operation->Parameter() ? HAS_PARAMETER : 0) | (operation->GetNotUsuallyCombinedWithHandle() != 0xFFFF ? NOT_USUALLY_COMBINED_WITH : 0);
    stream.WriteByte(flags);
    if (operation->Parameter())
    {
      stream << operation->Parameter();
    }
    if (flags & NOT_USUALLY_COMBINED_WITH)
    {
      stream << operation->GetNotUsuallyCombinedWithHandle();
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
