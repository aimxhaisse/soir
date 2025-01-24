#include <absl/log/log.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "core/dsp/engine.hh"
#include "core/dsp/track.hh"
#include "core/rt/bindings.hh"
#include "core/rt/engine.hh"
#include "soir.grpc.pb.h"

namespace py = pybind11;

namespace {

soir::rt::Engine* gRt_ = nullptr;
soir::dsp::Engine* gDsp_ = nullptr;

}  // namespace

namespace soir {
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
  m.doc() = "Soir Internal Live Module";

  m.def("set_bpm_", [](float bpm) { return gRt_->SetBPM(bpm); });
  m.def("get_bpm_", []() { return gRt_->GetBPM(); });
  m.def("get_beat_", []() {
    return static_cast<double>(gRt_->GetCurrentBeat() / 1000000.0);
  });

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
        case dsp::TRACK_SAMPLER:
          instrument = "sampler";
          break;

        case dsp::TRACK_MIDI_EXT:
          instrument = "midi_ext";
          break;

        default:
          instrument = "unknown";
          break;
      }

      result.push_back(
          py::dict("name"_a = track.name_, "muted"_a = track.muted_,
                   "volume"_a = track.volume_, "pan"_a = track.pan_,
                   "instrument"_a = instrument));
    }

    return result;
  });

  m.def("setup_tracks_", [](const py::dict& tracks) {
    std::list<dsp::TrackSettings> settings;

    for (auto& it : tracks) {
      dsp::TrackSettings s;

      auto name = it.first.cast<std::string>();
      auto track = it.second.cast<py::dict>();

      auto instr = track["instrument"].cast<std::string>();

      if (instr == "sampler") {
        s.instrument_ = dsp::TRACK_SAMPLER;
      } else if (instr == "midi_ext") {
        s.instrument_ = dsp::TRACK_MIDI_EXT;
      } else {
        LOG(ERROR) << "Unknown instrument: " << instr;
        return false;
      }

      s.name_ = name;
      s.muted_ = track["muted"].cast<std::optional<bool>>().value_or(false);
      s.volume_ = track["volume"].cast<std::optional<int>>().value_or(127);
      s.pan_ = track["pan"].cast<std::optional<int>>().value_or(64);
      s.extra_ = track["extra"].cast<std::optional<std::string>>().value_or("");

      auto fx = track["fx"].cast<py::dict>();

      //@HERE

      settings.push_back(s);
    }

    auto status = gDsp_->SetupTracks(settings);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to setup tracks: " << status;
      return false;
    }

    return true;
  });

  m.def("midi_note_on_", [](const std::string& track, uint8_t channel,
                            uint8_t note, uint8_t velocity) {
    gRt_->MidiNoteOn(track, channel, note, velocity);
  });
  m.def("midi_note_off_", [](const std::string& track, uint8_t channel,
                             uint8_t note, uint8_t velocity) {
    gRt_->MidiNoteOff(track, channel, note, velocity);
  });
  m.def("midi_cc_",
        [](const std::string& track, uint8_t channel, uint8_t cc,
           uint8_t value) { gRt_->MidiCC(track, channel, cc, value); });
  m.def("midi_sysex_sample_play_",
        [](const std::string& track, const std::string& p) {
          gRt_->MidiSysex(track, proto::MidiSysexInstruction::SAMPLER_PLAY, p);
        });
  m.def("midi_sysex_sample_stop_",
        [](const std::string& track, const std::string& p) {
          gRt_->MidiSysex(track, proto::MidiSysexInstruction::SAMPLER_STOP, p);
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
}  // namespace soir
