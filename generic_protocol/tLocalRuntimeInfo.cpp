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
/*!\file    plugins/network_transport/generic_protocol/tLocalRuntimeInfo.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-03-05
 *
 */
//----------------------------------------------------------------------
#include "plugins/network_transport/generic_protocol/tLocalRuntimeInfo.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"
#include "plugins/data_ports/tGenericPort.h"
#include "plugins/rpc_ports/tFuture.h"
#include "plugins/rpc_ports/tPromise.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/network_transport/runtime_info/tUriConnectorInfo.h"
#include "plugins/network_transport/runtime_info/tConnectorInfo.h"
#include "plugins/network_transport/generic_protocol/tNetworkPortInfoClient.h"
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

class tLocalRuntimeListenerBase : public core::tAnnotation, public core::tRuntimeListener
{
public:
  std::vector<std::shared_ptr<tLocalRuntimeInfo>> instances;
  tLocalRuntimeInfo::tSharedPortInfo shared_ports;

  void PublishChangeEvent(tLocalRuntimeInfo::tStructureChangeEvent* event)
  {
    std::shared_ptr<tLocalRuntimeInfo::tStructureChangeEvent> shared_event(event);
    for (auto & instance : instances)
    {
      // TODO: tSharedStructureChangeEvent could come from buffer pool
      std::unique_ptr<tLocalRuntimeInfo::tSharedStructureChangeEvent> to_enqueue(new tLocalRuntimeInfo::tSharedStructureChangeEvent(shared_event));
      instance->incoming_structure_changes.Enqueue(std::move(to_enqueue));
    }
  }

  void PublishChangeEvent(tLocalRuntimeInfo::tStructureChangeEvent* event, tNetworkTransportPlugin& plugin)
  {
    std::shared_ptr<tLocalRuntimeInfo::tStructureChangeEvent> shared_event(event);
    std::unique_ptr<tLocalRuntimeInfo::tSharedStructureChangeEvent> to_enqueue(new tLocalRuntimeInfo::tSharedStructureChangeEvent(shared_event));
    plugin.LocalRuntimeInfo()->incoming_structure_changes.Enqueue(std::move(to_enqueue));
  }
};

namespace
{

std::atomic<bool> serve_structure(false);

// Diverse structure change events

class tStructureCreatedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tStructureCreatedEvent(const core::tFrameworkElement& element) :
    tStructureChangeEventToPublish(element.GetHandle(), runtime_info::tLocalFrameworkElementInfo::IsSharedPort(element) ? runtime_info::tStructureExchange::SHARED_PORTS : runtime_info::tStructureExchange::COMPLETE_STRUCTURE),
    info(element)
  {}

  runtime_info::tLocalFrameworkElementInfo info;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tStructureCreatedMessage::Serialize(false, true, stream, local_handle);
    stream << info.static_info;
    if (info.static_info.IsDataPort())
    {
      stream << info.dynamic_info;
    }
    tStructureCreatedMessage::FinishMessage(stream);
  }
};

class tStructureChangedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tStructureChangedEvent(const core::tFrameworkElement& element) :
    tStructureChangeEventToPublish(element.GetHandle(), runtime_info::tLocalFrameworkElementInfo::IsSharedPort(element) ? runtime_info::tStructureExchange::SHARED_PORTS : runtime_info::tStructureExchange::COMPLETE_STRUCTURE),
    info(element)
  {}

  runtime_info::tLocalFrameworkElementInfo::tDynamicInfo info;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tStructureChangedMessage::Serialize(false, true, stream, local_handle);
    stream << info;
    tStructureCreatedMessage::FinishMessage(stream);
  }
};

class tStructureRemovedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tStructureRemovedEvent(const core::tFrameworkElement& element) :
    tStructureChangeEventToPublish(element.GetHandle(), runtime_info::tLocalFrameworkElementInfo::IsSharedPort(element) ? runtime_info::tStructureExchange::SHARED_PORTS : runtime_info::tStructureExchange::COMPLETE_STRUCTURE)
  {}

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tStructureDeletedMessage::Serialize(true, true, stream, local_handle);
  }
};

class tConnectorCreatedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tConnectorCreatedEvent(const core::tConnector& connector) :
    tStructureChangeEventToPublish(connector.Source().GetHandle(), runtime_info::tStructureExchange::FINSTRUCT),
    info(connector)
  {}

  runtime_info::tConnectorInfo info;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tConnectorCreatedMessage::Serialize(false, true, stream);
    stream << info;
    tStructureCreatedMessage::FinishMessage(stream);
  }
};

class tConnectorDeletedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tConnectorDeletedEvent(const core::tConnector& connector) :
    tStructureChangeEventToPublish(connector.Source().GetHandle(), runtime_info::tStructureExchange::FINSTRUCT),
    destination_handle(connector.Destination().GetHandle())
  {}

  core::tFrameworkElement::tHandle destination_handle;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tConnectorDeletedMessage::Serialize(true, true, stream, local_handle, destination_handle);
  }
};

class tUriConnectorCreatedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tUriConnectorCreatedEvent(const core::tUriConnector& connector) :
    tStructureChangeEventToPublish(connector.Owner().GetHandle(), runtime_info::tStructureExchange::FINSTRUCT),
    info(connector)
  {}

  runtime_info::tUriConnectorInfo info;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tUriConnectorCreatedMessage::Serialize(false, true, stream);
    stream << info;
    tStructureCreatedMessage::FinishMessage(stream);
  }
};

class tUriConnectorUpdatedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tUriConnectorUpdatedEvent(const core::tUriConnector& connector) :
    tStructureChangeEventToPublish(connector.Owner().GetHandle(), runtime_info::tStructureExchange::FINSTRUCT),
    index(runtime_info::tUriConnectorInfo::GetIndex(connector)),
    status(connector.GetStatus())
  {}

  uint8_t index;
  core::tUriConnector::tStatus status;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tUriConnectorUpdatedMessage::Serialize(true, true, stream, local_handle, index, status);
  }
};

class tUriConnectorDeletedEvent : public tLocalRuntimeInfo::tStructureChangeEventToPublish
{
public:
  tUriConnectorDeletedEvent(const core::tUriConnector& connector) :
    tStructureChangeEventToPublish(connector.Owner().GetHandle(), runtime_info::tStructureExchange::FINSTRUCT),
    index(runtime_info::tUriConnectorInfo::GetIndex(connector))
  {}

  uint8_t index;

  virtual void WriteToStream(rrlib::serialization::tOutputStream& stream) const override
  {
    tUriConnectorDeletedMessage::Serialize(true, true, stream, local_handle, index);
  }
};


class tInternalRuntimeListener : public tLocalRuntimeListenerBase
{
public:
  tInternalRuntimeListener()
  {
  }

  virtual void OnManagedDelete() override
  {
    core::tRuntimeEnvironment::GetInstance().RemoveListener(*this);
  }

  virtual void OnFrameworkElementChange(core::tRuntimeListener::tEvent change_type, core::tFrameworkElement& element) override
  {
    // Update shared ports?
    if (runtime_info::tLocalFrameworkElementInfo::IsSharedPort(element))
    {
      if (change_type == core::tRuntimeListener::tEvent::ADD || change_type == core::tRuntimeListener::tEvent::CHANGE)
      {
        shared_ports[element.GetHandle()] = runtime_info::tLocalFrameworkElementInfo(element);
      }
      else if (change_type == core::tRuntimeListener::tEvent::REMOVE)
      {
        shared_ports.erase(element.GetHandle());
      }
    }

    // Strategy of network client output port changed?
    if (element.GetFlag(core::tFrameworkElementFlag::NETWORK_ELEMENT))
    {
      tNetworkPortInfo* info = element.GetAnnotation<tNetworkPortInfo>();
      if (info && typeid(*info) == typeid(tNetworkPortInfoClient) && static_cast<core::tAbstractPort&>(element).IsOutputPort())
      {
        PublishChangeEvent(new tLocalRuntimeInfo::tClientPortStrategyChangeEvent(static_cast<core::tAbstractPort&>(element)), info->GetRemoteRuntime().NetworkTransport());
      }
    }

    // RPC port deletion?
    if (change_type == core::tRuntimeListener::tEvent::REMOVE && element.IsPort() && rpc_ports::IsRPCType(static_cast<core::tAbstractPort&>(element).GetDataType()))
    {
      PublishChangeEvent(new tLocalRuntimeInfo::tRpcPortDeleteEvent(element.GetHandle()));
    }

    // Publish structure change
    if (change_type != core::tRuntimeListener::tEvent::PRE_INIT && serve_structure.load() && (!element.GetFlag(core::tFrameworkElement::tFlag::NETWORK_ELEMENT)))
    {
      if (change_type == core::tRuntimeListener::tEvent::ADD)
      {
        PublishChangeEvent(new tStructureCreatedEvent(element));
      }
      else if (change_type == core::tRuntimeListener::tEvent::CHANGE)
      {
        PublishChangeEvent(new tStructureChangedEvent(element));
      }
      else if (change_type == core::tRuntimeListener::tEvent::REMOVE)
      {
        PublishChangeEvent(new tStructureRemovedEvent(element));
      }
    }
  }

  virtual void OnConnectorChange(tEvent change_type, core::tConnector& connector) override
  {
    // Publish structure change
    if (serve_structure.load() && (!connector.Flags().Get(core::tConnectionFlag::NON_PRIMARY_CONNECTOR)) && (!connector.Source().GetFlag(core::tFrameworkElement::tFlag::NETWORK_ELEMENT)) && (!connector.Destination().GetFlag(core::tFrameworkElement::tFlag::NETWORK_ELEMENT)))
    {
      if (change_type == core::tRuntimeListener::tEvent::ADD)
      {
        PublishChangeEvent(new tConnectorCreatedEvent(connector));
      }
      else if (change_type == core::tRuntimeListener::tEvent::REMOVE)
      {
        PublishChangeEvent(new tConnectorDeletedEvent(connector));
      }
    }
  }

  virtual void OnUriConnectorChange(tEvent change_type, core::tUriConnector& connector) override
  {
    // Publish structure change
    if (serve_structure.load() && (!connector.Flags().Get(core::tConnectionFlag::NON_PRIMARY_CONNECTOR)) && (!connector.Owner().GetFlag(core::tFrameworkElement::tFlag::NETWORK_ELEMENT)))
    {
      if (change_type == core::tRuntimeListener::tEvent::ADD)
      {
        PublishChangeEvent(new tUriConnectorCreatedEvent(connector));
      }
      else if (change_type == core::tRuntimeListener::tEvent::CHANGE)
      {
        PublishChangeEvent(new tUriConnectorUpdatedEvent(connector));
      }
      else if (change_type == core::tRuntimeListener::tEvent::REMOVE)
      {
        PublishChangeEvent(new tUriConnectorDeletedEvent(connector));
      }
    }
  }
};

tInternalRuntimeListener& GetInternalRuntimeListener()
{
  static tInternalRuntimeListener* instance = &core::tRuntimeEnvironment::GetInstance().EmplaceAnnotation<tInternalRuntimeListener>();
  return *instance;
}

}


std::string tLocalRuntimeInfo::runtime_name = "Unnamed runtime";

tLocalRuntimeInfo::tLocalRuntimeInfo(tNetworkTransportPlugin& plugin) :
  plugin(plugin)
{
}

tLocalRuntimeInfo::~tLocalRuntimeInfo()
{
}

void tLocalRuntimeInfo::Init(std::shared_ptr<tLocalRuntimeInfo>& self)
{
  rrlib::thread::tLock runtime_lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex());
  GetInternalRuntimeListener().instances.emplace_back(self);
}

bool tLocalRuntimeInfo::IsServingStructure()
{
  return serve_structure.load();
}

data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject> tLocalRuntimeInfo::OnPullRequest(data_ports::tGenericPort& origin, tRemoteRuntime& runtime)
{
  FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Pull request received");
  tPortSendCallEventPointer event_buffer = port_send_call_event_buffers.GetUnusedBuffer();
  if (!event_buffer)
  {
    event_buffer = port_send_call_event_buffers.AddBuffer(std::unique_ptr<tPortSendCallEvent>(new tPortSendCallEvent()));
  }
  event_buffer->pull_call.reset(new tPullCallInfo(runtime, *origin.GetAnnotation<tNetworkPortInfo>(), *origin.GetWrapped()));
  rpc_ports::tFuture<data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>> pull_future(event_buffer->pull_call->promise->GetFuture());
  incoming_calls_to_send.Enqueue(event_buffer);
  try
  {
    return pull_future.Get(data_ports::cPULL_TIMEOUT);
  }
  catch (const std::exception& exception)
  {
  }
  FINROC_LOG_PRINT(DEBUG_WARNING, "Pull call timed out (", origin, ")");
  return data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>();
}


const tLocalRuntimeInfo::tSharedPortInfo& tLocalRuntimeInfo::SharedPortInfo()
{
  return GetInternalRuntimeListener().shared_ports;
}

void tLocalRuntimeInfo::StartServingStructure()
{
  serve_structure = true;

  rrlib::thread::tLock runtime_lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex());
  auto& internal = GetInternalRuntimeListener();
  core::tRuntimeEnvironment::GetInstance().AddListener(internal);

  // Collect existing shared ports and store serialized information about them
  enum { cPORT_BUFFER_SIZE = 2048 };
  core::tAbstractPort* port_buffer[cPORT_BUFFER_SIZE];
  typename core::tFrameworkElement::tHandle start_handle = 0;
  while (true)
  {
    size_t port_count = core::tRuntimeEnvironment::GetInstance().GetAllPorts(port_buffer, cPORT_BUFFER_SIZE, start_handle);
    for (size_t i = 0; i < port_count; i++)
    {
      core::tAbstractPort& port = *port_buffer[i];
      if (runtime_info::tLocalFrameworkElementInfo::IsSharedPort(port))
      {
        internal.shared_ports.emplace(port.GetHandle(), runtime_info::tLocalFrameworkElementInfo(port));
      }
    }
    if (port_count < cPORT_BUFFER_SIZE)
    {
      break;
    }
    start_handle = port_buffer[cPORT_BUFFER_SIZE - 1]->GetHandle() + 1;
  };

  // Call OnStartServingStructure
  for (auto & instance : GetInternalRuntimeListener().instances)
  {
    instance->plugin.OnStartServingStructure();
  }
}

tLocalRuntimeInfo::tPullCallInfo::tPullCallInfo(tRemoteRuntime& remote_runtime, const tNetworkPortInfo& port_info, data_ports::common::tAbstractDataPort& local_port) :
  call_id(0),
  promise(new rpc_ports::tPromise<data_ports::tPortDataPointer<const rrlib::rtti::tGenericObject>>()), // TODO: optimize (avoid memory allocation)
  local_port_handle(local_port.GetHandle()),
  remote_port_handle(port_info.GetRemotePortHandle()),
  connection_handle(port_info.GetConnectionHandle()),
  timeout_time(rrlib::time::Now(false) + data_ports::cPULL_TIMEOUT),
  remote_runtime(&remote_runtime),
  message_flags((port_info.GetCurrentDynamicConnectionData().high_priority ? message_flags::cHIGH_PRIORITY : 0) | (port_info.IsServerPort() ? 0 : message_flags::cTO_SERVER))
{
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
