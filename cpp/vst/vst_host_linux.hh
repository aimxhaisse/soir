#pragma once

#include <mutex>
#include <vector>

#include "pluginterfaces/gui/iplugview.h"
#include "vst/vst_host.hh"

namespace soir {
namespace vst {

// Linux-specific host context that also implements IRunLoop so VSTGUI-based
// plug-ins can register event handlers and timers without crashing.
class LinuxHostContext : public HostContext, public Steinberg::Linux::IRunLoop {
 public:
  LinuxHostContext() = default;
  ~LinuxHostContext() override = default;

  // FUnknown — delegate to HostContext so both IHostApplication and IRunLoop
  // share the same reference count and queryInterface behaviour.
  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,
                                               void** obj) override;
  Steinberg::uint32 PLUGIN_API addRef() override;
  Steinberg::uint32 PLUGIN_API release() override;

  // IRunLoop
  Steinberg::tresult PLUGIN_API
  registerEventHandler(Steinberg::Linux::IEventHandler* handler,
                       Steinberg::Linux::FileDescriptor fd) override;
  Steinberg::tresult PLUGIN_API
  unregisterEventHandler(Steinberg::Linux::IEventHandler* handler) override;
  Steinberg::tresult PLUGIN_API
  registerTimer(Steinberg::Linux::ITimerHandler* handler,
                Steinberg::Linux::TimerInterval milliseconds) override;
  Steinberg::tresult PLUGIN_API
  unregisterTimer(Steinberg::Linux::ITimerHandler* handler) override;

 protected:
  Steinberg::tresult QueryPlatformInterface(const Steinberg::TUID iid,
                                            void** obj) override;

 private:
  std::mutex runloop_mutex_;
  struct EventHandlerEntry {
    Steinberg::IPtr<Steinberg::Linux::IEventHandler> handler;
    Steinberg::Linux::FileDescriptor fd;
  };
  struct TimerHandlerEntry {
    Steinberg::IPtr<Steinberg::Linux::ITimerHandler> handler;
    Steinberg::Linux::TimerInterval interval;
  };
  std::vector<EventHandlerEntry> event_handlers_;
  std::vector<TimerHandlerEntry> timer_handlers_;
};

}  // namespace vst
}  // namespace soir
