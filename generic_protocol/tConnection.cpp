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
/*!\file    plugins/network_transport/generic_protocol/tConnection.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tConnection.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tRemoteRuntime.h"
#include "plugins/network_transport/generic_protocol/tNetworkTransportPlugin.h"

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
namespace generic_protocol
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

const std::shared_ptr<tConnection> cEMPTY_POINTER;

}


//----------------------------------------------------------------------
// tConnection constructors
//----------------------------------------------------------------------
tConnection::tConnection(bool double_buffered_writing) :
  last_ack_request_index(-1),
  last_acknowledged_packet(0),
  next_packet_index(1),
  sent_bulk_packets(0),
  sent_packet_data(),
  received_data_after_last_connect(false),
  double_buffered_writing(double_buffered_writing),
  write_buffers({ rrlib::serialization::tMemoryBuffer(), rrlib::serialization::tMemoryBuffer(double_buffered_writing ? rrlib::serialization::tMemoryBuffer::cDEFAULT_SIZE : 0) }),
              current_write_stream(),
              back_buffer_locked(false),
              closed(false)
{
  sent_packet_data.fill(tSentPacketData(rrlib::time::cNO_TIME, 0));
}

//----------------------------------------------------------------------
// tConnection destructor
//----------------------------------------------------------------------
tConnection::~tConnection()
{
}

void tConnection::Close()
{
  if (!closed)
  {
    CloseImplementation();
    closed = true;
    if (GetRemoteRuntime())
    {
      GetRemoteRuntime()->ManagedDelete();
    }
  }

}

size_t tConnection::ProcessIncomingMessageBatch(const rrlib::serialization::tFixedBuffer& buffer, size_t start_at)
{
  try
  {
    rrlib::serialization::tMemoryBuffer mem_buffer(const_cast<char*>(buffer.GetPointer()), buffer.Capacity());
    rrlib::serialization::tInputStream stream(mem_buffer, shared_connection_info->input_stream_prototype);

    // process acknowledging
    if (start_at == 0)
    {
      // packet acknowledgment request
      int16_t ack_request_index = stream.ReadShort();
      if (ack_request_index >= 0)
      {
        last_ack_request_index = ack_request_index;
      }

      // packet acknowledgments
      int16_t acknowledgement = stream.ReadShort();
      if (acknowledgement >= 0)
      {
        last_acknowledged_packet = acknowledgement;
      }
    }
    else
    {
      stream.Skip(start_at);
    }

    while (stream.MoreDataAvailable())
    {
      size_t command_start_position = static_cast<size_t>(stream.GetAbsoluteReadPosition());
      tOpCode op_code = stream.ReadEnum<tOpCode>();
      if (op_code >= tOpCode::OTHER)
      {
        FINROC_LOG_PRINT(WARNING, "Received corrupted TCP message batch. Invalid opcode. Skipping.");
        return 0;
      }
      size_t message_size = message_size_for_opcodes[static_cast<size_t>(op_code)].ReadMessageSize(stream);
      if (message_size == 0 || message_size > stream.Remaining())
      {
        FINROC_LOG_PRINT(WARNING, "Received corrupted TCP message batch. Invalid message size: ", message_size, ". Skipping.");
        return 0;
      }
      rrlib::serialization::tMemoryBuffer message_buffer(mem_buffer.GetBufferPointer(static_cast<size_t>(stream.GetAbsoluteReadPosition())), message_size);
      stream.Skip(message_size);

      try
      {
        if (op_code != tOpCode::PEER_INFO)
        {
          bool defer = shared_connection_info->remote_runtime->ProcessMessage(op_code, message_buffer, *this);
          if (defer)
          {
            return command_start_position;
          }
        }
        else
        {
          ProcessPeerInfoMessage(message_buffer);
        }
      }
      catch (const std::exception& ex)
      {
        FINROC_LOG_PRINT(WARNING, "Failed to deserialize message of type ", make_builder::GetEnumString(op_code), ". Skipping. Cause: ", ex);
      }
    }
  }
  catch (const std::exception& ex)
  {
    FINROC_LOG_PRINT(WARNING, "Error processing message batch: ", ex, ". Skipping.");
  }
  return 0;
}

void tConnection::ProcessPeerInfoMessage(rrlib::serialization::tMemoryBuffer& buffer)
{
  throw std::runtime_error("PEER_INFO OpCode not implemented in this network transport");
}

void tConnection::SendPendingMessages(const rrlib::time::tTimestamp& time_now)
{
  if (double_buffered_writing && back_buffer_locked) // back buffer is still written - we cannot write anything else during this time
  {
    return;
  }

  // Determine whether more packages can be sent (ugly: counter wrap-arounds)
  int non_acknowledged_express_packets = static_cast<int>(next_packet_index - 1) - last_acknowledged_packet;
  if (non_acknowledged_express_packets < 0)
  {
    non_acknowledged_express_packets += 0x8000;
  }
  assert(non_acknowledged_express_packets >= 0 && non_acknowledged_express_packets <= cMAX_NOT_ACKNOWLEDGED_PACKETS);
  bool more_express_packets_allowed = non_acknowledged_express_packets < cMAX_NOT_ACKNOWLEDGED_PACKETS;

  tSentPacketData& last_acknowledged = sent_packet_data[last_acknowledged_packet % sent_packet_data.size()];
  int non_acknowledged_bulk_packets = static_cast<int>(sent_bulk_packets) - last_acknowledged.second;
  if (non_acknowledged_bulk_packets < 0)
  {
    non_acknowledged_bulk_packets += 0x8000;
  }
  assert(GetRemoteRuntime());
  bool more_bulk_packets_allowed = non_acknowledged_bulk_packets < static_cast<int>(GetRemoteRuntime()->NetworkTransport().par_max_not_acknowledged_packets_bulk.Get());

  if (non_acknowledged_express_packets >= cMAX_NOT_ACKNOWLEDGED_PACKETS)
  {
    return;
  }

  // Send new port data
  if (!ports_with_data_to_send.empty())
  {
    if (WritePortDataToSendNowToStream(current_write_stream, time_now, more_bulk_packets_allowed))
    {
      sent_bulk_packets++;
      if (sent_bulk_packets < 0)
      {
        sent_bulk_packets = 0;
      }
    }
  }

  bool connection_for_express_data = this->GetRemoteRuntime()->GetExpressConnection().get() == this;
  bool connection_for_bulk_data = this->GetRemoteRuntime()->GetPrimaryConnection().get() == this;

  // Send packet
  current_write_stream.Flush();
  if (write_buffers[0].GetSize() > 8 || last_ack_request_index >= 0)
  {

    // update internal acknowledgement management variables
    bool data_packet = more_express_packets_allowed && write_buffers[0].GetSize() > 8;
    int16_t ack_request = -1;
    if (data_packet)
    {
      ack_request = next_packet_index;
      tSentPacketData& current_packet_data = sent_packet_data[next_packet_index % sent_packet_data.size()];
      current_packet_data.first = time_now;
      current_packet_data.second = sent_bulk_packets;

      next_packet_index++;
      if (next_packet_index < 0)
      {
        next_packet_index = 0;
      }
    }

    // send message packet
    write_buffers[0].GetBuffer().PutInt(0, write_buffers[0].GetSize() - 4); // put size
    write_buffers[0].GetBuffer().PutShort(4, ack_request); // put ack request
    write_buffers[0].GetBuffer().PutShort(6, last_ack_request_index); // put ack response
    last_ack_request_index = -1;
    if (double_buffered_writing)
    {
      std::swap(write_buffers[0], write_buffers[1]);
    }

    std::shared_ptr<tConnection> connection = this->GetRemoteRuntime()->GetConnection(connection_for_express_data);
    if (connection.get() == this)
    {
      SendMessagePacket(connection, write_buffers[double_buffered_writing ? 1 : 0], back_buffer_locked);
    }
    else
    {
      FINROC_LOG_PRINT(ERROR, "Could not get shared pointer on connection");
    }

    // prepare new packet with messages
    InitFrontBuffer();
    WriteLocalRegisterUpdatesToStream(current_write_stream);
  }

  // Check round-trip times: Is critical threshold exceeded?
  if (received_data_after_last_connect && non_acknowledged_express_packets)
  {
    rrlib::time::tTimestamp first_non_acknowledged_send_time = sent_packet_data[(last_acknowledged_packet + 1) % sent_packet_data.size()].first;
    if (time_now - first_non_acknowledged_send_time > GetRemoteRuntime()->NetworkTransport().par_critical_ping_threshold.Get())
    {
      FINROC_LOG_PRINT(WARNING, "Critical network roundtrip time exceeded. Notifying ports.");
      for (auto sub_element = GetRemoteRuntime()->SubElementsBegin(); sub_element != GetRemoteRuntime()->SubElementsEnd(); ++sub_element)
      {
        if (sub_element->IsPort())
        {
          core::tAbstractPort& port = static_cast<core::tAbstractPort&>(*sub_element);
          tNetworkPortInfo* info = port.GetAnnotation<tNetworkPortInfo>();
          if (info && info->GetCurrentDynamicConnectionData().high_priority ? connection_for_express_data : connection_for_bulk_data)
          {
            for (auto connected_port = port.OutgoingConnectionsBegin(); connected_port != port.OutgoingConnectionsEnd(); ++connected_port)
            {
              if (connected_port->Destination().IsChildOf(*GetRemoteRuntime()))
              {
                // conversion port -> another step
                for (auto connected_port2 = connected_port->Destination().OutgoingConnectionsBegin(); connected_port2 != connected_port->Destination().OutgoingConnectionsEnd(); ++connected_port2)
                {
                  connected_port2->Destination().NotifyOfNetworkConnectionLoss();
                }
              }
              else
              {
                connected_port->Destination().NotifyOfNetworkConnectionLoss();
              }
            }
          }
        }
      }
      received_data_after_last_connect = false;
    }
  }
}

bool tConnection::WritePortDataToSendNowToStream(rrlib::serialization::tOutputStream& stream, const rrlib::time::tTimestamp& time_now, bool low_priority_data_allowed)
{
  bool result = false;
  size_t keep_count = 0;
  for (auto it = ports_with_data_to_send.begin(); it != ports_with_data_to_send.end(); ++it)
  {
    tNetworkPortInfo& port = **it;
    if ((port.GetCurrentDynamicConnectionData().high_priority || low_priority_data_allowed) && port.GetLastUpdate() + port.GetUpdateInterval() <= time_now)
    {
      port.WriteDataBuffersToStream(current_write_stream, time_now);
      result |= (!port.GetCurrentDynamicConnectionData().high_priority);
    }
    else
    {
      ports_with_data_to_send[keep_count] = *it; // keep port in list for later sending
      keep_count++;
    }
  }

  ports_with_data_to_send.resize(keep_count, nullptr);

  return result;
}

void tConnection::WriteLocalRegisterUpdatesToStream(rrlib::serialization::tOutputStream& stream)
{
  if (stream.IsPublishedRegisterUpdatePending())
  {
    tTypeUpdateMessage message;
    message.Serialize(false, true, stream);
    stream << rrlib::rtti::tType();
    stream.WriteShort(0);
    message.FinishMessage(stream);
  }
}

tConnection::tSharedConnectionInfo::tSharedConnectionInfo(tLocalRuntimeInfo& local_runtime_info) :
  remote_runtime(nullptr),
  local_runtime_info(local_runtime_info),
  initial_reading_complete(false),
  initial_writing_complete(false),
  initial_structure_reading_complete(false),
  initial_structure_writing_complete(false),
  framework_elements_in_full_structure_exchange_sent_until_handle(0)
{}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
