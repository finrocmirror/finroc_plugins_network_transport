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
/*!\file    plugins/network_transport/generic_protocol/tLocalRuntimeInfo.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-05
 *
 * \brief   Contains tLocalRuntimeInfo
 *
 * \b tLocalRuntimeInfo
 *
 * Stores and manages information on local runtime environment.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__network_transport__generic_protocol__tLocalRuntimeInfo_h__
#define __plugins__network_transport__generic_protocol__tLocalRuntimeInfo_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueable.h"
#include "rrlib/buffer_pools/tBufferPool.h"
#include "core/tFrameworkElement.h"
#include "plugins/data_ports/tPort.h"
#include "plugins/rpc_ports/internal/tRPCPort.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tUriConnectorData.h"

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
class tNetworkPortInfo;
class tRemoteRuntime;
class tNetworkServerPort;
class tNetworkTransportPlugin;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Info on local runtime environment
/*!
 * Stores and manages information on local runtime environment.
 * As structure changes may occur concurrently, enqueues them in a concurrent queue and
 * stores them for the plugin's thread for processing.
 * All public methods may be called concurrently.
 */
class tLocalRuntimeInfo
{
  //! Port change event
  /*!
   * Whenever value of an observed network port changes, this is stored in such an event.
   */
  struct tPortDataChangeEvent :
    public rrlib::buffer_pools::tBufferManagementInfo,
    public rrlib::concurrent_containers::tQueueable<rrlib::concurrent_containers::tQueueability::MOST_OPTIMIZED>,
    public rrlib::buffer_pools::tNotifyOnRecycle
  {

    /*! New value of port */
    data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> new_value;

    /*! Network port info object of network port */
    tNetworkPortInfo* network_port_info;

    /*! Change type */
    data_ports::tChangeStatus change_type;


    tPortDataChangeEvent() :
      new_value(),
      network_port_info(nullptr),
      change_type(data_ports::tChangeStatus::NO_CHANGE)
    {}

    void OnRecycle()
    {
      new_value.Reset();
    }
  };

  /*!
   * Stores information on pull call to be answered by connection partner
   */
  struct tPullCallInfo
  {
    /*! Call id of pull call */
    rpc_ports::internal::tCallId call_id;

    /*! Promise for waiting thread */
    std::shared_ptr<rpc_ports::tPromise<data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>>> promise;

    /*! Handle of local port that requested pull operation */
    tFrameworkElementHandle local_port_handle;

    /*! Handle of remote port */
    tFrameworkElementHandle remote_port_handle;

    /*! Handle of connection */
    tFrameworkElementHandle connection_handle;

    /*! Time when call times out */
    rrlib::time::tTimestamp timeout_time;

    /*! Pointer to remote part */
    tRemoteRuntime* remote_runtime;

    /*! Connection flags */
    uint8_t message_flags;


    tPullCallInfo(tRemoteRuntime& remote_runtime, const tNetworkPortInfo& port_info, data_ports::common::tAbstractDataPort& local_port);

    /*! Used as boost asio handler to forward this pull call to TCP Thread */
    void operator()();
  };


  //! Port send call event
  /*!
   * Whenever value a network port receives a send command, this is stored in such an event.
   */
  struct tPortSendCallEvent :
    public rrlib::buffer_pools::tBufferManagementInfo,
    public rrlib::concurrent_containers::tQueueable<rrlib::concurrent_containers::tQueueability::MOST_OPTIMIZED>,
    public rrlib::buffer_pools::tNotifyOnRecycle
  {
    /*! Pointer to client's server port - if this is an RPC call */
    tNetworkServerPort* server_port;

    /*! Pointer containing pull - if this is a pull call */
    std::unique_ptr<tPullCallInfo> pull_call;


    tPortSendCallEvent() :
      server_port(nullptr)
    {}

    void OnRecycle()
    {
      server_port = nullptr;
      pull_call.reset();
    }
  };

  typedef rrlib::buffer_pools::tBufferPool < tPortDataChangeEvent, rrlib::concurrent_containers::tConcurrency::MULTIPLE_READERS,
          rrlib::buffer_pools::management::QueueBased, rrlib::buffer_pools::deleting::CollectGarbage,
          rrlib::buffer_pools::recycling::UseOwnerStorageInBuffer > tPortDataChangeEventPool;

  typedef rrlib::buffer_pools::tBufferPool < tPortSendCallEvent, rrlib::concurrent_containers::tConcurrency::MULTIPLE_READERS,
          rrlib::buffer_pools::management::QueueBased, rrlib::buffer_pools::deleting::CollectGarbage,
          rrlib::buffer_pools::recycling::UseOwnerStorageInBuffer > tPortSendCallEventPool;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef typename tPortDataChangeEventPool::tPointer tPortDataChangeEventPointer;
  typedef typename tPortSendCallEventPool::tPointer tPortSendCallEventPointer;
  typedef std::map<core::tFrameworkElement::tHandle, runtime_info::tLocalFrameworkElementInfo> tSharedPortInfo;

  /*!
   * Structure change event base class
   */
  class tStructureChangeEvent
  {
  public:
    virtual ~tStructureChangeEvent() = default;
  protected:
    tStructureChangeEvent() = default;
  };

  /*! Event that RPC port was deleted (so that buffer pools created in connection for these ports can be deleted) */
  class tRpcPortDeleteEvent : public tStructureChangeEvent
  {
  public:

    tRpcPortDeleteEvent(tFrameworkElementHandle handle) :
      handle(handle)
    {}

    tFrameworkElementHandle handle;
  };

  /*!
   * Structure change event to send to clients. Subclasses are defined in .cpp file
   */
  class tStructureChangeEventToPublish : public tStructureChangeEvent
  {
  public:

    tStructureChangeEventToPublish(core::tFrameworkElement::tHandle local_handle, runtime_info::tStructureExchange minimum_relevant_level) :
      local_handle(local_handle),
      minimum_relevant_level(minimum_relevant_level)
    {}

    /*!
     * Local handle (of framework element or connector source/owner).
     * This is used to filter events that should be sent to client during initial structure exchange.
     */
    const core::tFrameworkElement::tHandle local_handle;

    /*! Level for which this structure change is relevant */
    const runtime_info::tStructureExchange minimum_relevant_level;


    /*!
     * Write structure change event to specified output stream
     *
     * \param stream Stream to write change to
     */
    virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const = 0;
  };

  /*!
   * Strategy change event of network client port
   */
  class tClientPortStrategyChangeEvent : public tStructureChangeEvent
  {
  public:
    tClientPortStrategyChangeEvent(core::tAbstractPort& port) : port(port)
    {}

    core::tAbstractPort& port;  // Port is stored here to avoid issues with ports being deleted until event is processed
  };

  /*!
   * Wrapper to be able to enqueue events in multiple queues
   */
  class tSharedStructureChangeEvent : public rrlib::concurrent_containers::tQueueable<rrlib::concurrent_containers::tQueueability::MOST_OPTIMIZED>
  {
  public:
    tSharedStructureChangeEvent(const std::shared_ptr<tStructureChangeEvent>& shared_event) : event(shared_event)
    {}

    std::shared_ptr<tStructureChangeEvent> event;
  };

  typedef rrlib::concurrent_containers::tQueue <std::unique_ptr<tSharedStructureChangeEvent>, rrlib::concurrent_containers::tConcurrency::SINGLE_READER_AND_WRITER, rrlib::concurrent_containers::tDequeueMode::ALL> tIncomingStructureChangesQueue;


  /*!
   * \param plugin Reference to network plugin
   */
  tLocalRuntimeInfo(tNetworkTransportPlugin& plugin);

  ~tLocalRuntimeInfo();

  /*!
   * \return Name of local runtime environment. Will be displayed in tooling and status messages. Does not need to be unique. Typically the program/process name.
   */
  static const std::string& GetName()
  {
    return runtime_name;
  }

  /*!
   * \return Returns true as soon as structure info can be served.
   *         This may be enabled later, as this can cause quite a lot of overhead if done during
   *         construction and startup of large applications.
   */
  static bool IsServingStructure();

  /*!
   * Called whenever a data port receives a pull request
   */
  data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> OnPullRequest(data_ports::tGenericPort& origin, tRemoteRuntime& runtime);

  /*!
   * Called whenever a client-side server port receives data to send
   */
  void OnIncomingRpcCall(tNetworkServerPort& port_with_calls_to_send)
  {
    tPortSendCallEventPointer event_buffer = port_send_call_event_buffers.GetUnusedBuffer();
    if (!event_buffer)
    {
      event_buffer = port_send_call_event_buffers.AddBuffer(std::unique_ptr<tPortSendCallEvent>(new tPortSendCallEvent()));
    }
    event_buffer->server_port = &port_with_calls_to_send;
    incoming_calls_to_send.Enqueue(event_buffer);
    // TODO: Wakeup network thread immediately as in Finroc 14.08 ?
  }

  /*!
   * Called (via port listener) whenever a network port receives data to send
   */
  void OnPortChange(data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>& value, tNetworkPortInfo* info, data_ports::tChangeContext& change_context)
  {
    tPortDataChangeEventPointer event_buffer = port_data_change_event_buffers.GetUnusedBuffer();
    if (!event_buffer)
    {
      event_buffer = port_data_change_event_buffers.AddBuffer(std::unique_ptr<tPortDataChangeEvent>(new tPortDataChangeEvent()));
    }
    event_buffer->new_value = std::move(value);
    event_buffer->network_port_info = info;
    event_buffer->change_type = change_context.ChangeType();
    incoming_port_data_changes.Enqueue(event_buffer);
  }

  /*!
   * \param new_name Name of local runtime environment. Will be displayed in tooling and status messages. Does not need to be unique. Typically the program/process name.
   */
  static void SetName(const std::string& new_name)
  {
    runtime_name = new_name;
  }

  /*!
   * \return Info on shared ports in this runtime environment.
   *         Must be accessed with runtime structure lock (typically makes sense when publishing anyway - to avoid race conditions with concurrent structure changes).
   */
  static const tSharedPortInfo& SharedPortInfo();

  /*!
   * Calling this method makes IsServingStructure() return 'true'
   */
  static void StartServingStructure();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tUriConnector;
  friend class tNetworkTransportPlugin;
  friend class tLocalRuntimeListenerBase;
  friend class tRemoteRuntime;

  /*! Reference to network plugin */
  tNetworkTransportPlugin& plugin;

  /*!
   * Concurrent queue with incoming structure changes.
   * Queue is filled when runtime changes occur (structure mutex is acquired by thread adding data).
   * Queue is processed by network transport thread
   */
  tIncomingStructureChangesQueue incoming_structure_changes;

  /*! Concurrent queue with incoming port value changes */
  rrlib::concurrent_containers::tQueue < tPortDataChangeEventPointer, rrlib::concurrent_containers::tConcurrency::MULTIPLE_WRITERS,
        rrlib::concurrent_containers::tDequeueMode::ALL > incoming_port_data_changes;

  /*! Buffer pool with port value change event buffers */
  tPortDataChangeEventPool port_data_change_event_buffers;

  /*! Concurrent queue with incoming port value changes */
  rrlib::concurrent_containers::tQueue < tPortSendCallEventPointer, rrlib::concurrent_containers::tConcurrency::MULTIPLE_WRITERS,
        rrlib::concurrent_containers::tDequeueMode::ALL > incoming_calls_to_send;

  /*! Buffer pool with port value change event buffers */
  tPortSendCallEventPool port_send_call_event_buffers;

  /*! Name of local runtime environment. Will be displayed in tooling and status messages. Does not need to be unique. Typically the program/process name. */
  static std::string runtime_name;


  /*! Initialize object */
  void Init(std::shared_ptr<tLocalRuntimeInfo>& self);

  /*!
   * Event that URI connector (of this protocol) has been added
   */
  class tUriConnectorAddEvent : public tStructureChangeEvent, public tUriConnectorData
  {
  public:
    tUriConnectorAddEvent(const tUriConnectorData& data, const rrlib::rtti::conversion::tConversionOperationSequence& local_conversion) : tUriConnectorData(data), local_conversion(local_conversion)
    {}

    rrlib::rtti::conversion::tConversionOperationSequence local_conversion;
  };

  /*!
   * Event that URI connector (of this protocol) has been changed
   */
  class tUriConnectorChangeEvent : public tStructureChangeEvent
  {
  public:

    tUriConnectorChangeEvent(const tUriConnectorData& data) :
      uri_connector_address(data.UriConnectorAddress()),
      dynamic_parameters(data.DynamicParameters())
    {}

    const void* uri_connector_address;
    tDynamicConnectorParameters dynamic_parameters;
  };

  /*!
   * Event that URI connector (of this protocol) has been deleted
   */
  class tUriConnectorDeleteEvent : public tStructureChangeEvent
  {
  public:

    tUriConnectorDeleteEvent(const tUriConnectorData& data) :
      uri_connector_address(data.UriConnectorAddress())
    {}

    const void* uri_connector_address;
  };
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
