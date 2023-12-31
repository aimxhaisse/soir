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

PYBIND11_EMBEDDED_MODULE(__live, m) {
  m.doc() = "Maethstro Internal Live Module";

  m.def("__set_bpm", [](int bpm) { return gEngine_->SetBPM(bpm); });
  m.def("__get_bpm", []() { return gEngine_->GetBPM(); });
  m.def("__get_user", []() { return gEngine_->GetUser(); });
  m.def("__log", [](const std::string& message) {
    gEngine_->Log(gEngine_->GetUser(), message);
  });
}

}  // namespace midi
}  // namespace maethstro
