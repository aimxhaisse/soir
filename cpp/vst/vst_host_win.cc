#include "vst/vst_host_win.hh"

namespace soir {
namespace vst {

HostContext* CreateHostContext() { return new WindowsHostContext(); }

}  // namespace vst
}  // namespace soir
