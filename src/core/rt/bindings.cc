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

PYBIND11_EMBEDDED_MODULE(live_, m) {
  m.doc() = "Neon Internal Live Module";

  m.def("set_bpm_", [](float bpm) { return gRt_->SetBPM(bpm); });
  m.def("get_bpm_", []() { return gRt_->GetBPM(); });
  m.def("get_beat_",
        []() { return static_cast<double>(gRt_->GetCurrentBeat() / 1000000); });

  m.def("log_", [](const std::string& message) { gRt_->Log(message); });

  m.def("schedule_", [](float beats, py::function func) {
    gRt_->Schedule(gRt_->GetCurrentBeat() + beats * 1000000, func);
  });

  py::class_<bindings::PyTrack>(m, "Track")
      .def(py::init<>())
      .def_readwrite("instrument", &bindings::PyTrack::instrument)
      .def_readwrite("channel", &bindings::PyTrack::channel)
      .def_readwrite("muted", &bindings::PyTrack::muted)
      .def_readwrite("volume", &bindings::PyTrack::volume)
      .def_readwrite("pan", &bindings::PyTrack::pan)
      .def("__repr__", [](const bindings::PyTrack& track) {
        return "<Track instrument='" + track.instrument +
               "' channel=" + std::to_string(track.channel) + ">";
      });

  m.def("get_tracks_", []() {
    std::list<dsp::TrackSettings> tracks;
    std::vector<bindings::PyTrack> result;

    auto status = gDsp_->GetTracks(&tracks);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get tracks: " << status;
      return result;
    }

    for (const auto& track : tracks) {
      bindings::PyTrack py_track;

      py_track.channel = track.channel_;
      py_track.muted = track.muted_;
      py_track.volume = track.volume_;
      py_track.pan = track.pan_;

      switch (track.instrument_) {
        case dsp::TRACK_MONO_SAMPLER:
          py_track.instrument = "mono_sampler";
          break;

        default:
          py_track.instrument = "unknown";
          break;
      }

      result.push_back(py_track);
    }

    return result;
  });

  m.def("setup_tracks_", [](const std::vector<bindings::PyTrack>& tracks) {
    std::list<dsp::TrackSettings> settings;

    for (auto& track : tracks) {
      dsp::TrackSettings s;

      if (track.instrument == "mono_sampler") {
        s.instrument_ = dsp::TRACK_MONO_SAMPLER;
      } else {
        LOG(ERROR) << "Unknown instrument: " << track.instrument;
        return false;
      }

      s.channel_ = track.channel;
      s.muted_ = track.muted.value_or(false);
      s.volume_ = track.volume.value_or(127);
      s.pan_ = track.pan.value_or(64);

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
  m.def("get_samples_from_pack_", [](const std::string& p) {
    auto pack = gDsp_->GetSampleManager().GetPack(p);

    if (pack == nullptr) {
      return std::vector<std::string>();
    }

    return pack->GetSampleNames();
  });
}

}  // namespace rt
}  // namespace neon
