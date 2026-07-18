#pragma once

#include "vst/vst_host.hh"

namespace soir {
namespace vst {

// Windows-specific host context.
class WindowsHostContext : public HostContext {
 public:
  WindowsHostContext() = default;
  ~WindowsHostContext() override = default;
};

}  // namespace vst
}  // namespace soir
