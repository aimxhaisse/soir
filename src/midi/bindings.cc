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

PYBIND11_EMBEDDED_MODULE(live_, m) {
  m.doc() = "Maethstro Internal Live Module";

  m.def("set_bpm_", [](float bpm) { return gEngine_->SetBPM(bpm); });
  m.def("get_bpm_", []() { return gEngine_->GetBPM(); });
  m.def("get_user_", []() { return gEngine_->GetUser(); });
  m.def("get_beat_", []() {
    return static_cast<double>(gEngine_->GetCurrentBeat() / 1000000);
  });

  m.def("log_", [](const std::string& user, const std::string& message) {
    gEngine_->Log(user, message);
  });

  m.def("schedule_", [](float beats, py::function func) {
    gEngine_->Schedule(gEngine_->GetCurrentBeat() + beats * 1000000, func);
  });

  m.def("midi_note_on_", [](uint8_t channel, uint8_t note, uint8_t velocity) {
    gEngine_->MidiNoteOn(channel, note, velocity);
  });
  m.def("midi_note_off_", [](uint8_t channel, uint8_t note, uint8_t velocity) {
    gEngine_->MidiNoteOff(channel, note, velocity);
  });
  m.def("midi_cc_", [](uint8_t channel, uint8_t cc, uint8_t value) {
    gEngine_->MidiCC(channel, cc, value);
  });
}

}  // namespace midi
}  // namespace maethstro
