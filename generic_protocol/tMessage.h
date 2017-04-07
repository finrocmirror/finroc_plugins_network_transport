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
/*!\file    plugins/network_transport/generic_protocol/tMessage.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-01-12
 *
 * \brief   Contains tMessage
 *
 * \b tMessage
 *
 * Single message sent via the TCP protocol.
 * Typedef'ed for every opcode in in protocol_definitions.h.
 * This class is used mainly to serialize and deserialize data to the network streams.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tMessage_h__
#define __plugins__network_transport__generic_protocol__tMessage_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/serialization/serialization.h"
#include "core/log_messages.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/definitions.h"

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

enum class tMessageSize
{
  FIXED,                   // fixed size message (calculated from TArgs)
  VARIABLE_UP_TO_255_BYTE, // variable message size up to 255 bytes
  VARIABLE_UP_TO_4GB       // variable message size up to 4GB
};

enum { cDEBUG_MESSAGE_TERMINATOR = 0xCD };

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Single TCP message
/*!
 * Single message sent via the TCP protocol.
 * Typedef'ed for every opcode in in protocol_definitions.h.
 * This class is used mainly to serialize and deserialize data to the network streams.
 *
 * \tparam Topcode Opcode to message to send
 * \tparam Tsize Size of message (see enum)
 * \tparam Targs Arguments of message
 */
template <typename TOpcode, TOpcode Topcode, tMessageSize Tsize, typename ... Targs>
class tMessage : private rrlib::util::tNoncopyable
{
  /*! Size of all arguments */
  template <bool ZERO_ARGS = (sizeof...(Targs) == 0)>
  inline constexpr static typename std::enable_if < !ZERO_ARGS, int >::type ArgumentsSize()
  {
    return CalculateArgumentSize<Targs...>();
  }
  template <bool ZERO_ARGS = (sizeof...(Targs) == 0)>
  inline constexpr static typename std::enable_if<ZERO_ARGS, int>::type ArgumentsSize()
  {
    return 0;
  }

  /*! Helper for ArgumentsSize() */
  template <typename TArg>
  inline constexpr static int CalculateArgumentSize()
  {
    return (std::is_enum<TArg>::value ? 1 : sizeof(TArg));
  }
  template <typename TArg, typename TArg2, typename ... TRest>
  inline constexpr static int CalculateArgumentSize()
  {
    return (std::is_enum<TArg>::value ? 1 : sizeof(TArg)) + tMessage::CalculateArgumentSize<TArg2, TRest...>();
  }

public:

  enum { cARGUMENT_SIZE = ArgumentsSize() };

  tMessage() :
    parameters()
  {}


  /*! Deserialize - excluding opcode and size */
  inline void Deserialize(rrlib::serialization::tInputStream& stream, bool finish = true)
  {
    stream >> parameters;
    if (finish)
    {
      FinishDeserialize(stream);
    }
  }

  inline void FinishDeserialize(rrlib::serialization::tInputStream& stream)
  {
    if (stream.GetSourceInfo().custom_info & runtime_info::cDEBUG_PROTOCOL)
    {
      uint8_t debug_number;
      stream >> debug_number;
      if (debug_number != cDEBUG_MESSAGE_TERMINATOR)
      {
        throw std::runtime_error("Message not correctly terminated");
      }
    }
  }

  static void FinishMessage(rrlib::serialization::tOutputStream& stream)
  {
    if (stream.GetTargetInfo().custom_info & runtime_info::cDEBUG_PROTOCOL)
    {
      stream << static_cast<uint8_t>(cDEBUG_MESSAGE_TERMINATOR);
    }
    if (Tsize != tMessageSize::FIXED)
    {
      stream.SkipTargetHere();
    }
  }

  /*!
   * \return Deserialized argument with specified index
   */
  template <size_t INDEX>
  inline typename std::tuple_element<INDEX, std::tuple<Targs...>>::type& Get()
  {
    return std::get<INDEX>(parameters);
  }

  static constexpr tMessageSize MessageSize()
  {
    return Tsize;
  }

  /*!
   * Serialize - including opcode and size
   *
   * \param finish_message Is message complete after writing args? (If not, FinishMessage() must be called manually)
   * \param write_opcode Whether to write opcode
   */
  static inline void Serialize(bool finish_message, bool write_opcode, rrlib::serialization::tOutputStream& stream, const Targs& ... args)
  {
    FINROC_LOG_PRINT_STATIC(DEBUG_VERBOSE_1, "Sending message ", make_builder::GetEnumString(Topcode));
    if (write_opcode)
    {
      stream << Topcode;
    }
    if (Tsize != tMessageSize::FIXED)
    {
      stream.WriteSkipOffsetPlaceholder(Tsize == tMessageSize::VARIABLE_UP_TO_255_BYTE);
    }

    std::tuple<const Targs& ...> args_tuple(args...);
    stream << args_tuple;
    if (finish_message)
    {
      FinishMessage(stream);
    }
  }

private:

  /*! parameters in this message */
  std::tuple<Targs...> parameters;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
