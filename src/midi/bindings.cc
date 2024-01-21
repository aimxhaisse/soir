#include <absl/log/log.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

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
    proto::GetTracks_Response response;
    std::vector<bindings::PyTrack> result;

    auto status = gEngine_->GetTracks(&response);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get tracks: " << status;
      return result;
    }

    for (const auto& track : response.tracks()) {
      bindings::PyTrack py_track;

      py_track.channel = track.channel();
      py_track.muted = track.muted();
      py_track.volume = track.volume();
      py_track.pan = track.pan();

      switch (track.instrument()) {
        case proto::Track::TRACK_MONO_SAMPLER:
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
    proto::SetupTracks_Request request;
    proto::SetupTracks_Response response;

    for (auto& track : tracks) {
      proto::Track proto_track;

      if (track.instrument == "mono_sampler") {
        proto_track.set_instrument(proto::Track::TRACK_MONO_SAMPLER);
      } else {
        LOG(ERROR) << "Unknown instrument: " << track.instrument;
        return false;
      }

      proto_track.set_channel(track.channel);

      if (track.muted.has_value()) {
        proto_track.set_muted(track.muted.value());
      }
      if (track.volume.has_value()) {
        proto_track.set_volume(track.volume.value());
      }
      if (track.pan.has_value()) {
        proto_track.set_pan(track.pan.value());
      }

      request.add_tracks()->CopyFrom(proto_track);
    }

    auto status = gEngine_->SetupTracks(request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to setup tracks: " << status;
      return false;
    }

    return true;
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
