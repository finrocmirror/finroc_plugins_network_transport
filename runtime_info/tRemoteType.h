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
/*!\file    plugins/network_transport/runtime_info/tRemoteType.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-26
 *
 * \brief   Contains tRemoteType
 *
 * \b tRemoteType
 *
 * Represents type in remote runtime environment.
 * To be used with rrlib::serialization::PublishedRegisters (handles serialization etc.).
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__runtime_info__tRemoteType_h__
#define __plugins__network_transport__runtime_info__tRemoteType_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/serialization/PublishedRegisters.h"
#include "rrlib/rtti/rtti.h"

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
//! Remote rrlib_rtti data type
/*!
 * Represents type in remote runtime environment.
 * To be used with rrlib::serialization::PublishedRegisters (handles serialization etc.).
 */
class tRemoteType : public rrlib::serialization::PublishedRegisters::tRemoteEntryBase<uint16_t, rrlib::rtti::detail::tTypeInfo::tSharedInfo::tRegisteredTypes>
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tRemoteType() : type_traits(0), types_checked(0), underlying_type(0)
  {}

  void DeserializeRegisterEntry(rrlib::serialization::tInputStream& stream);

  static void SerializeRegisterEntry(rrlib::serialization::tOutputStream& stream, const rrlib::rtti::tType& type);

  /*!
   * \return Local data type that is equivalent to this type. Empty type if there is no equivalent local type.
   */
  rrlib::rtti::tType GetLocalDataType() const;

  /*!
   * \return Name of remote type
   */
  const std::string& GetName() const
  {
    return name;
  }

  /*!
   * \return Bit vector of type traits determined at compile time (see rrlib::rtti::tType::GetTypeTraits())
   */
  inline uint32_t GetTypeTraits() const
  {
    return type_traits;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Type traits of remote type */
  uint32_t type_traits;

  /*! Local data type that represents the same type; null-type if there is no such type in local runtime environment */
  mutable rrlib::rtti::tType local_data_type;

  /*! Number of local types checked to resolve type */
  mutable uint types_checked;

  /*! Name of remote type */
  std::string name;

  /*! Uid of underlying type */
  uint16_t underlying_type;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}

#endif
