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
/*!\file    plugins/network_transport/generic_protocol/tConnection.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-28
 *
 * \brief   Contains tConnection
 *
 * \b tConnection
 *
 * Base class for a connection between two peers that is able to send and receive messages.
 * It should be subclassed for specific network protocols.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tConnection_h__
#define __plugins__network_transport__generic_protocol__tConnection_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/tPort.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/definitions.h"
#include "plugins/network_transport/generic_protocol/tLocalRuntimeInfo.h"

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
class tRemoteRuntime;
class tNetworkServerPort;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Connection
/*!
 * Base class for a single connection between two peers that is able to send and receive messages.
 * It should be subclassed for specific network protocols.
 */
class tConnection : public rrlib::util::tNoncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Constructor for primary connection
   *
   * \param local_runtime_info Reference to info on local runtime to use
   */
  tConnection(tLocalRuntimeInfo& local_runtime_info, bool double_buffered_writing) : tConnection(double_buffered_writing)
  {
    shared_connection_info.reset(new tSharedConnectionInfo(local_runtime_info));
  }

  /*!
   * Constructor for secondary connections
   *
   * \param primary_connection Reference to primary connection to partner peer.
   */
  tConnection(tConnection& primary_connection, bool double_buffered_writing) : tConnection(double_buffered_writing)
  {
    shared_connection_info = primary_connection.shared_connection_info;
  }


  virtual ~tConnection();


  /*! Closes connection and deletes any remote runtime object */
  void Close();

  /*!
   * \return Write stream for current connection. Should not be accessed concurrently for thread-safety.
   */
  rrlib::serialization::tOutputStream& CurrentWriteStream()
  {
    return current_write_stream;
  }

  /*!
   * Enqueue port with data to send.
   * Network thread will send any data in this port in SendPendingMessages() function
   */
  void EnqueuePortWithDataToSend(tNetworkPortInfo& network_port_info)
  {
    ports_with_data_to_send.push_back(&network_port_info);
  }

  /*!
   * \return Local runtime info
   */
  tLocalRuntimeInfo& GetLocalRuntimeInfo()
  {
    return shared_connection_info->local_runtime_info;
  }

  std::string GetLogDescription() const;

  /*!
   * \return Pointer to remote runtime environment that this connection is attached to
   */
  tRemoteRuntime* GetRemoteRuntime()
  {
    return shared_connection_info->remote_runtime;
  }

  /*!
   * \return Wether connection has been closed
   */
  bool IsClosed() const
  {
    return closed;
  }

  /*!
   * Processes incoming message batch in read buffer
   *
   * \param buffer Raw buffer containing all messages (sent with SendPendingMessages)  Start reading at specified offset.
   * \param start_at Start reading at specified offset. Can be greater than zero if processing has been deferred.
   * \return Zero, if message batch was processed completely. Offset to continue processing at, if processing is deferred.
   */
  size_t ProcessIncomingMessageBatch(const rrlib::serialization::tFixedBuffer& buffer, size_t start_at = 0);

  /*!
   * Send pending messages.
   * Swaps front buffer and back buffer. Reinitializes current write stream.
   */
  void SendPendingMessages(const rrlib::time::tTimestamp& time_now);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
protected:

  /*! Initializes beginning of front buffer */
  void InitFrontBuffer()
  {
    current_write_stream.Reset(write_buffers[0], shared_connection_info->output_stream_prototype);
    current_write_stream.WriteInt(0); // Placeholder for size
    current_write_stream.WriteShort(0); // Placeholder for ack requests
    current_write_stream.WriteShort(0); // Placeholder for acks
  }

  /*!
   * Contains information shared among primary and non-primary connections to a specific peer
   */
  struct tSharedConnectionInfo
  {
    /*! Prototype stream with configuration to deserialize data from connection partner. Contains remote registers. */
    rrlib::serialization::tInputStream input_stream_prototype;

    /*! Prototype stream with configuration to serialize data for connection partner. Handles transfer of register entries. */
    rrlib::serialization::tOutputStream output_stream_prototype;

    /*! Data on remote runtime environment that this connection is connected to */
    tRemoteRuntime* remote_runtime;

    /*! Info on local runtime environment */
    tLocalRuntimeInfo& local_runtime_info;

    /*! Have initial reading and writing been completed? */
    bool initial_reading_complete, initial_writing_complete;

    /*! Have initial structure reading and writing been completed? */
    bool initial_structure_reading_complete, initial_structure_writing_complete;

    /*!
     * In case connection partner wants full or finstruct structure exchange:
     * Until which handle have structure elements been sent to connection partner?
     * (excludes element with handle of this variable)
     * (invalid as soon as initial_structure_writing_complete is true)
     */
    uint64_t framework_elements_in_full_structure_exchange_sent_until_handle;

    static_assert(sizeof(framework_elements_in_full_structure_exchange_sent_until_handle) > sizeof(core::tFrameworkElement::tHandle), "Must be larger");


    tSharedConnectionInfo(tLocalRuntimeInfo& local_runtime_info);
  };

  /*! Contains information shared among primary and non-primary connections to a specific peer */
  tSharedConnectionInfo& SharedConnectionInfo()
  {
    return *shared_connection_info;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tRemoteRuntime;

  /*! Contains information shared among primary and non-primary connections to a specific peer */
  std::shared_ptr<tSharedConnectionInfo> shared_connection_info;

  /*!
   * Index of last acknowledgement request that was received and has to be sent
   * -1 if there are currently no pending acknowledgement requests
   */
  int16_t last_ack_request_index;

  /*! Index of last sent packet that was acknowledged by connection partner (0-0x7FFF)*/
  int16_t last_acknowledged_packet;

  /*! Index of next packet that is to be sent */
  int16_t next_packet_index;

  /*! Number of sent bulk packages */
  int16_t sent_bulk_packets;

  /*! Data on sent packet: Time sent, bulk packets sent */
  typedef std::pair<rrlib::time::tTimestamp, int16_t> tSentPacketData;

  /*! Data on packet n that was sent (Index is n % MAX_NOT_ACKNOWLEDGED_PACKETS => efficient and safe implementation (ring queue)) */
  std::array < tSentPacketData, cMAX_NOT_ACKNOWLEDGED_PACKETS + 1 > sent_packet_data;

  /*! Ports that currently have data to send via this connection */
  std::vector<tNetworkPortInfo*> ports_with_data_to_send;

  /*!
   * Has any data been received since connection was (last) established?
   * Set to true whenever data is received.
   * If max. ping threshold is exceeded, ports are notified of disconnect and this variable is reset.
   * The purpose of this variable is that ports are notified only once after ping threshold is exceeded.
   */
  bool received_data_after_last_connect;

  /*! Whether double-buffered writing is enabled */
  const bool double_buffered_writing;

  /*!
   * Element zero is front buffer for writing
   * Element one is back buffer for writing (if double-buffering is enabled)
   * If double-buffering is enabled they are swapped on every write operation
   */
  std::array<rrlib::serialization::tMemoryBuffer, 2> write_buffers;

  /*! Current stream for writing data to */
  rrlib::serialization::tOutputStream current_write_stream;

  /*! True, while back buffer is locked (e.g. written to stream). During this time, no further buffers message packets can be sent (SendPendingMessages will return immediately) */
  bool back_buffer_locked;

  /*! True after connection has been closed */
  bool closed;


  tConnection(bool double_buffered_writing);


  /*!
   * Implementation of Close()
   */
  virtual void CloseImplementation() = 0;

  /*!
   * Process protocol-specific PEER_INFO message
   */
  virtual void ProcessPeerInfoMessage(rrlib::serialization::tMemoryBuffer& buffer);

  /*!
   * Sends readily prepared packet with messages over this connection to partner.
   * This method must be overridden for each network transport.
   *
   * \param self Shared pointer to self. As long as this is owned, it is safe to write to 'back_buffer_lock_variable'
   * \param buffer_to_send Buffer with readily prepared packet with messages
   * \param back_buffer_lock_variable Variable that indicates whether back buffer is currently locked. Method should set this to true as long as 'buffer_to_send' must not be modified.
   */
  virtual void SendMessagePacket(std::shared_ptr<tConnection>& self, const rrlib::serialization::tMemoryBuffer& buffer_to_send, bool& back_buffer_lock_variable) = 0;

  /*!
   * Writes any updates on auto-updated local registers to stream
   *
   * \param stream Stream to write messages to
   */
  void WriteLocalRegisterUpdatesToStream(rrlib::serialization::tOutputStream& stream);

  /*!
   * Writes *PORT_VALUE_CHANGE* messages to stream from ports in 'ports_with_data_to_send'.
   *
   * \param stream Stream to write messages to
   * \param time_now Current time
   * \param low_priority_data_allowed Whether low priority data may be included (otherwise these ports are skipped)
   * \return Whether any data from low priority data was written to stream
   */
  bool WritePortDataToSendNowToStream(rrlib::serialization::tOutputStream& stream, const rrlib::time::tTimestamp& time_now, bool low_priority_data_allowed);
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
