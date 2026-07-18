#include "vst/vst_host_linux.hh"

namespace soir {
namespace vst {

using namespace Steinberg;

tresult PLUGIN_API
LinuxHostContext::registerEventHandler(Steinberg::Linux::IEventHandler* handler,
                                       Steinberg::Linux::FileDescriptor fd) {
  std::lock_guard<std::mutex> lock(runloop_mutex_);
  // The caller retains its own reference to the handler; use shared() to
  // addRef rather than owned() so our entry holds a proper reference.
  // With owned() the handler would be one reference short and get deleted
  // while the plug-in's run loop wrapper still references it (use-after-free
  // during editor teardown).
  event_handlers_.push_back({Steinberg::shared(handler), fd});
  return kResultOk;
}

tresult PLUGIN_API LinuxHostContext::unregisterEventHandler(
    Steinberg::Linux::IEventHandler* handler) {
  std::lock_guard<std::mutex> lock(runloop_mutex_);
  auto it = std::remove_if(
      event_handlers_.begin(), event_handlers_.end(),
      [handler](const auto& entry) { return entry.handler.get() == handler; });
  event_handlers_.erase(it, event_handlers_.end());
  return kResultOk;
}

tresult PLUGIN_API
LinuxHostContext::registerTimer(Steinberg::Linux::ITimerHandler* handler,
                                Steinberg::Linux::TimerInterval milliseconds) {
  std::lock_guard<std::mutex> lock(runloop_mutex_);
  // See registerEventHandler: shared() is required for correct ref-counting.
  timer_handlers_.push_back({Steinberg::shared(handler), milliseconds});
  return kResultOk;
}

tresult PLUGIN_API
LinuxHostContext::unregisterTimer(Steinberg::Linux::ITimerHandler* handler) {
  std::lock_guard<std::mutex> lock(runloop_mutex_);
  auto it = std::remove_if(
      timer_handlers_.begin(), timer_handlers_.end(),
      [handler](const auto& entry) { return entry.handler.get() == handler; });
  timer_handlers_.erase(it, timer_handlers_.end());
  return kResultOk;
}

tresult PLUGIN_API LinuxHostContext::queryInterface(const TUID iid,
                                                    void** obj) {
  return HostContext::queryInterface(iid, obj);
}

uint32 PLUGIN_API LinuxHostContext::addRef() { return HostContext::addRef(); }

uint32 PLUGIN_API LinuxHostContext::release() { return HostContext::release(); }

tresult LinuxHostContext::QueryPlatformInterface(const TUID iid, void** obj) {
  if (FUnknownPrivate::iidEqual(iid, Steinberg::Linux::IRunLoop::iid)) {
    *obj = static_cast<Steinberg::Linux::IRunLoop*>(this);
    HostContext::addRef();
    return kResultOk;
  }
  *obj = nullptr;
  return kNoInterface;
}

HostContext* CreateHostContext() { return new LinuxHostContext(); }

}  // namespace vst
}  // namespace soir
