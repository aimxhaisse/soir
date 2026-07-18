#pragma once

#include "vst/vst_host.hh"

namespace soir {
namespace vst {

// macOS-specific host context. macOS VSTGUI does not require IRunLoop.
class MacOSHostContext : public HostContext {
 public:
  MacOSHostContext() = default;
  ~MacOSHostContext() override = default;
};

}  // namespace vst
}  // namespace soir
