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
/*!\file    plugins/network_transport/generic_protocol/tNetworkPortInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-02-22
 *
 * \brief   Contains tNetworkPortInfo
 *
 * \b tNetworkPortInfo
 *
 * Port annotation storing information on network ports.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__tcp__internal__tNetworkPortInfo_h__
#define __plugins__tcp__internal__tNetworkPortInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tFrameworkElement.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tRemoteRuntime.h"

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

typedef typename core::tFrameworkElement::tHandle tHandle;
class tNetworkPortInfoClient;
class tRemoteRuntime;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Network port information
/*!
 * Port annotation storing information on network ports.
 */
class tNetworkPortInfo : public core::tAnnotation
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tNetworkPortInfo(tRemoteRuntime& remote_runtime, tHandle connection_handle, tHandle remote_port_handle, int16_t strategy, core::tAbstractPort& port, tHandle served_port_handle = 0);

  /*!
   * Changes strategy of this (remote) port
   *
   * \param new_strategy New Strategy to set
   */
  void ChangeStrategy(int16_t new_strategy);

  /*!
   * \return Returns client info if this is a client port
   */
  tNetworkPortInfoClient* GetClientInfo()
  {
    return client_info.get();
  }

  /*!
   * \return Port handle in remote runtime environment
   */
  tHandle GetConnectionHandle() const
  {
    return connection_handle;
  }

  /*!
   * \return Current dynamic connection parameters
   */
  const tDynamicConnectionData& GetCurrentDynamicConnectionData() const
  {
    return current_dynamic_connection_data;
  }

  /*!
   * \return Last time port data was written to network
   */
  rrlib::time::tTimestamp GetLastUpdate()
  {
    return last_update;
  }

  /*!
   * \return Port handle in remote runtime environment
   */
  tHandle GetRemotePortHandle() const
  {
    return remote_port_handle;
  }

  /*!
   * \return Remote runtime that port belongs to
   */
  tRemoteRuntime& GetRemoteRuntime()
  {
    return remote_runtime;
  }

  /*!
   * \return In case this is a server port: Local handle of served port - otherwise 0
   */
  tHandle GetServedPortHandle() const
  {
    return served_port_handle;
  }

  /*!
   * \return Minimum update interval for this port
   */
  rrlib::time::tDuration GetUpdateInterval()
  {
    return std::chrono::milliseconds(40); // TODO
  }

  /*!
   * \return Is this a server port?
   */
  bool IsServerPort() const
  {
    return !client_info;
  }

  void OnPortChange(data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>& value, data_ports::tChangeContext& change_context)
  {
    //FINROC_LOG_PRINT(DEBUG, "Port Changed ", port.GetWrapped()->GetQualifiedName(), " ", rrlib::serialization::Serialize(*value));
    if (strategy > 0)
    {
      remote_runtime.GetLocalRuntimeInfo().OnPortChange(value, this, change_context);
    }
  }

  /*!
   * Serializes buffer to binary stream using specified encoding.
   * If data compression is specified but not available, binary encoding is used.
   *
   * \param stream Stream to serialize to
   * \param buffer Buffer to serialize
   * \param encoding Desired data encoding
   */
  inline void SerializeBuffer(rrlib::serialization::tOutputStream& stream, data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>& buffer, message_flags::tDataEncoding encoding)
  {
    buffer->Serialize(stream, static_cast<rrlib::serialization::tDataEncoding>(encoding));
  }

  /*!
   * Sets subscription data of server ports to specified values
   */
  void SetServerSideDynamicConnectionData(const tDynamicConnectionData& data);

  /*!
   * Writes all values to send to provided stream.
   * Resets values to send afterwards
   */
  void WriteDataBuffersToStream(rrlib::serialization::tOutputStream& stream, const rrlib::time::tTimestamp& time_now)
  {
    typename tLocalRuntimeInfo::tPortDataChangeEventPointer value_to_send = values_to_send.Dequeue();
    //FINROC_LOG_PRINT(ERROR, "a");
    if (value_to_send)
    {
      bool express_data = current_dynamic_connection_data.high_priority;
      bool legacy = stream.GetTargetInfo().revision == 0;
      uint8_t message_flags = desired_encoding | ((!legacy) && client_info ? message_flags::cTO_SERVER : 0);
      int handle = stream.GetTargetInfo().revision == 0 ? remote_port_handle : connection_handle;
      if (express_data && values_to_send.Size() == 0)
      {
        // We only have one value to send
        if (value_to_send->new_value.GetTimestamp() == rrlib::time::cNO_TIME)
        {
          //FINROC_LOG_PRINT(WARNING, GetAnnotated<core::tAbstractPort>()->GetQualifiedName());
          tSmallPortValueChangeWithoutTimestamp::Serialize(false, true, stream, handle, message_flags);
          stream << value_to_send->change_type;
          value_to_send->new_value->Serialize(stream, static_cast<rrlib::serialization::tDataEncoding>(desired_encoding)); // express data is not compressed
          stream.WriteBoolean(false);
          tSmallPortValueChangeWithoutTimestamp::FinishMessage(stream);
        }
        else
        {
          tSmallPortValueChange::Serialize(false, true, stream, handle, message_flags);
          stream << value_to_send->change_type << value_to_send->new_value.GetTimestamp();
          value_to_send->new_value->Serialize(stream, static_cast<rrlib::serialization::tDataEncoding>(desired_encoding)); // express data is not compressed
          stream.WriteBoolean(false);
          tSmallPortValueChange::FinishMessage(stream);
        }
      }
      else
      {
        //FINROC_LOG_PRINT(ERROR, "a2");
        tPortValueChange::Serialize(false, true, stream, handle, message_flags);
        do
        {
          stream << value_to_send->change_type << value_to_send->new_value.GetTimestamp();
          SerializeBuffer(stream, value_to_send->new_value, desired_encoding); // data might be compressed
          value_to_send = values_to_send.Dequeue();
          stream.WriteBoolean(value_to_send.get());
        }
        while (value_to_send);
        tPortValueChange::FinishMessage(stream);
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tNetworkPortInfoClient;
  friend class tNetworkTransportPlugin;
  friend class tRemoteRuntime;

  /*! Handle of connection */
  const tHandle connection_handle;

  /*! Port handle in remote runtime environment */
  const tHandle remote_port_handle;

  /*! Strategy to use for this port - if it is destination port */
  int16_t strategy;

  /*! Current dynamic connection parameters */
  tDynamicConnectionData current_dynamic_connection_data;

  /*! Remote runtime that port belongs to */
  tRemoteRuntime& remote_runtime;

  /*! FIFO queue with values to send to connection partners */
  rrlib::concurrent_containers::tQueue < typename tLocalRuntimeInfo::tPortDataChangeEventPointer, rrlib::concurrent_containers::tConcurrency::NONE,
        rrlib::concurrent_containers::tDequeueMode::FIFO, true > values_to_send;

  /*! Has port been deleted? (non-volatile, because ports are only deleted by TCP thread and this value is only checked by TCP thread) */
  bool deleted;

  /*! Last time port data was written to network */
  rrlib::time::tTimestamp last_update;

  /*! Desired encoding of network partner */
  message_flags::tDataEncoding desired_encoding;

  /*! In case this is a server port: Local handle of served port - otherwise 0 */
  const tHandle served_port_handle;

  /*! Contains client info if this is a client port */
  std::unique_ptr<tNetworkPortInfoClient> client_info;


  virtual void OnManagedDelete() override;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
