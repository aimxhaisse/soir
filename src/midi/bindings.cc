#include <absl/log/log.h>
#include <pybind11/embed.h>

#include "bindings.hh"
#include "engine.hh"

namespace py = pybind11;

namespace {

maethstro::midi::Engine* gEngine_ = nullptr;

}  // namespace

namespace maethstro {
namespace midi {

absl::Status bindings::SetEngine(Engine* engine) {
  if (gEngine_ != nullptr) {
    LOG(ERROR) << "Engine already initialized, unable to run multiple "
                  "instances at the same time";
    return absl::InternalError("Engine already initialized");
  }
  gEngine_ = engine;

  return absl::OkStatus();
}

void bindings::ResetEngine() {
  gEngine_ = nullptr;
}

PYBIND11_EMBEDDED_MODULE(live, m) {
  m.doc() = "Maethstro L I V E";

  m.def(
      "set_bpm",
      [](int bpm) {
        gEngine_->Live_SetBPM(bpm);
        return gEngine_->Live_GetBPM();
      },
      "Sets the BPM");

  m.def(
      "get_bpm", []() { return gEngine_->Live_GetBPM(); }, "Retrieves the BPM");

  m.def(
      "get_user", []() { return gEngine_->Live_GetUser(); },
      "Retrieves the current user");

  m.def(
      "log",
      [](const std::string& message) {
        gEngine_->Live_Log(gEngine_->Live_GetUser(), message);
      },
      "Logs a message");
}

}  // namespace midi
}  // namespace maethstro
