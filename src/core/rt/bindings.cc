#include <absl/log/log.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "core/dsp/engine.hh"
#include "core/dsp/track.hh"
#include "core/rt/bindings.hh"
#include "core/rt/engine.hh"
#include "neon.grpc.pb.h"

namespace py = pybind11;

namespace {

neon::rt::Engine* gRt_ = nullptr;
neon::dsp::Engine* gDsp_ = nullptr;

}  // namespace

namespace neon {
namespace rt {

using namespace pybind11::literals;

absl::Status bindings::SetEngines(rt::Engine* rt, dsp::Engine* dsp) {
  if (gRt_ != nullptr) {
    LOG(ERROR) << "Engines already initialized, unable to run multiple "
                  "instances at the same time";
    return absl::InternalError("Engine already initialized");
  }
  gRt_ = rt;
  gDsp_ = dsp;

  return absl::OkStatus();
}

void bindings::ResetEngines() {
  gRt_ = nullptr;
  gDsp_ = nullptr;
}

PYBIND11_EMBEDDED_MODULE(bindings, m) {
  m.doc() = "Neon Internal Live Module";

  m.def("set_bpm_", [](float bpm) { return gRt_->SetBPM(bpm); });
  m.def("get_bpm_", []() { return gRt_->GetBPM(); });
  m.def("get_beat_",
        []() { return static_cast<double>(gRt_->GetCurrentBeat() / 1000000); });

  m.def("log_", [](const std::string& message) { gRt_->Log(message); });

  m.def("schedule_", [](float beats, py::function func) {
    gRt_->Schedule(gRt_->GetCurrentBeat() + beats * 1000000, func);
  });

  m.def("get_tracks_", []() {
    std::list<dsp::TrackSettings> tracks;
    std::vector<py::dict> result;

    auto status = gDsp_->GetTracks(&tracks);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get tracks: " << status;
      return result;
    }

    for (const auto& track : tracks) {
      std::string instrument;

      switch (track.instrument_) {
        case dsp::TRACK_MONO_SAMPLER:
          instrument = "mono_sampler";
          break;

        case dsp::TRACK_MIDI_EXT:
          instrument = "midi_ext";
          break;

        default:
          instrument = "unknown";
          break;
      }

      result.push_back(
          py::dict("channel"_a = track.channel_, "muted"_a = track.muted_,
                   "volume"_a = track.volume_, "pan"_a = track.pan_,
                   "instrument"_a = instrument));
    }

    return result;
  });

  m.def("setup_tracks_", [](const std::vector<py::dict>& tracks) {
    std::list<dsp::TrackSettings> settings;

    for (auto& track : tracks) {
      dsp::TrackSettings s;

      auto instr = track["instrument"].cast<std::string>();

      if (instr == "mono_sampler") {
        s.instrument_ = dsp::TRACK_MONO_SAMPLER;
      } else if (instr == "midi_ext") {
        s.instrument_ = dsp::TRACK_MIDI_EXT;
      } else {
        LOG(ERROR) << "Unknown instrument: " << instr;
        return false;
      }

      s.channel_ = track["channel"].cast<int>();
      s.muted_ = track["muted"].cast<std::optional<bool>>().value_or(false);
      s.volume_ = track["volume"].cast<std::optional<int>>().value_or(127);
      s.pan_ = track["pan"].cast<std::optional<int>>().value_or(64);

      settings.push_back(s);
    }

    auto status = gDsp_->SetupTracks(settings);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to setup tracks: " << status;
      return false;
    }

    return true;
  });

  m.def("midi_note_on_", [](uint8_t channel, uint8_t note, uint8_t velocity) {
    gRt_->MidiNoteOn(channel, note, velocity);
  });
  m.def("midi_note_off_", [](uint8_t channel, uint8_t note, uint8_t velocity) {
    gRt_->MidiNoteOff(channel, note, velocity);
  });
  m.def("midi_cc_", [](uint8_t channel, uint8_t cc, uint8_t value) {
    gRt_->MidiCC(channel, cc, value);
  });
  m.def("midi_sysex_sample_play_", [](uint8_t channel, const std::string& p) {
    gRt_->MidiSysex(channel, proto::MidiSysexInstruction::SAMPLER_PLAY, p);
  });
  m.def("midi_sysex_sample_stop_", [](uint8_t channel, const std::string& p) {
    gRt_->MidiSysex(channel, proto::MidiSysexInstruction::SAMPLER_STOP, p);
  });

  m.def("get_packs_",
        []() { return gDsp_->GetSampleManager().GetPackNames(); });
  m.def("get_samples_", [](const std::string& p) {
    auto pack = gDsp_->GetSampleManager().GetPack(p);

    if (pack == nullptr) {
      return std::vector<std::string>();
    }

    return pack->GetSampleNames();
  });

  m.def("get_code_", []() { return gRt_->GetCode(); });
}

}  // namespace rt
}  // namespace neon
