#include <absl/log/log.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <unistd.h>

#include "core/audio_output.hh"
#include "core/dsp.hh"
#include "core/engine.hh"
#include "core/inst/midi_ext.hh"
#include "core/rt/bindings.hh"
#include "core/portaudio.hh"
#include "core/rt/runtime.hh"
#include "core/track.hh"
#include "soir.grpc.pb.h"

namespace py = pybind11;

namespace {

soir::rt::Runtime* gRt_ = nullptr;
soir::Engine* gDsp_ = nullptr;

}  // namespace

namespace soir {
namespace rt {

using namespace pybind11::literals;

absl::Status bindings::SetEngines(rt::Runtime* rt, Engine* dsp) {
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

Engine* bindings::GetDsp() {
  return gDsp_;
}

rt::Runtime* bindings::GetRt() {
  return gRt_;
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
    std::list<Track::Settings> tracks;
    std::vector<py::dict> result;

    auto status = gDsp_->GetTracks(&tracks);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get tracks: " << status;
      return result;
    }

    for (const auto& track : tracks) {
      std::string instrument;

      switch (track.instrument_) {
        case inst::Type::SAMPLER:
          instrument = "sampler";
          break;

        case inst::Type::MIDI_EXT:
          instrument = "midi_ext";
          break;

        default:
          instrument = "unknown";
          break;
      }

      std::list<std::string> fxs;
      for (const auto& fx : track.fxs_) {
        std::string type;

        switch (fx.type_) {
          case fx::Type::CHORUS:
            type = "chorus";
            break;

          case fx::Type::REVERB:
            type = "reverb";
            break;

          case fx::Type::LPF:
            type = "lpf";
            break;

          case fx::Type::HPF:
            type = "hpf";
            break;

          default:
            type = "unknown";
            break;
        }

        fxs.push_back(type);
      }

      result.push_back(
          py::dict("name"_a = track.name_, "muted"_a = track.muted_,
                   "volume"_a = track.volume_.Raw(), "pan"_a = track.pan_.Raw(),
                   "instrument"_a = instrument, "fxs"_a = fxs));
    }

    return result;
  });

  m.def("setup_tracks_", [](const py::dict& tracks) {
    std::list<Track::Settings> settings;
    Controls* ctrls = gDsp_->GetControls();

    for (auto& it : tracks) {
      Track::Settings s;

      auto name = it.first.cast<std::string>();
      auto track = it.second.cast<py::dict>();

      auto instr = track["instrument"].cast<std::string>();

      if (instr == "sampler") {
        s.instrument_ = inst::Type::SAMPLER;
      } else if (instr == "midi_ext") {
        s.instrument_ = inst::Type::MIDI_EXT;
      } else {
        LOG(ERROR) << "Unknown instrument: " << instr;
        return false;
      }

      s.name_ = name;
      s.muted_ = track["muted"].cast<std::optional<bool>>().value_or(false);
      s.volume_ = Parameter::FromPyDict(ctrls, track, "volume");
      s.pan_ = Parameter::FromPyDict(ctrls, track, "pan");
      s.extra_ = track["extra"].cast<std::optional<std::string>>().value_or("");

      s.volume_.SetRange(0.0f, 1.0f);
      s.pan_.SetRange(-1.0f, 1.0f);

      auto fxs = track["fxs"].cast<std::list<py::dict>>();
      for (auto it : fxs) {
        fx::Fx::Settings fx_settings;

        fx_settings.name_ = it["name"].cast<std::string>();
        fx_settings.mix_ = it["mix"].cast<std::optional<float>>().value_or(1.0);
        fx_settings.extra_ =
            it["extra"].cast<std::optional<std::string>>().value_or("");

        if (it["type"].cast<std::string>() == "chorus") {
          fx_settings.type_ = fx::Type::CHORUS;
        } else if (it["type"].cast<std::string>() == "reverb") {
          fx_settings.type_ = fx::Type::REVERB;
        } else if (it["type"].cast<std::string>() == "lpf") {
          fx_settings.type_ = fx::Type::LPF;
        } else if (it["type"].cast<std::string>() == "hpf") {
          fx_settings.type_ = fx::Type::HPF;
        } else {
          fx_settings.type_ = fx::Type::UNKNOWN;
        }

        s.fxs_.push_back(fx_settings);
      }

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

  m.def("controls_get_frequency_update_",
        []() { return kControlsFrequencyUpdate; });

  m.def("midi_sysex_update_controls_", [](const std::string& p) {
    gRt_->MidiSysex(std::string(kInternalControls),
                    proto::MidiSysexInstruction::UPDATE_CONTROLS, p);
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

  m.def("get_audio_out_devices_", []() {
    std::vector<portaudio::Device> pa_devices;
    auto status = portaudio::GetAudioOutDevices(&pa_devices);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get audio output devices: " << status;
      return std::vector<std::pair<int, std::string>>();
    }
    
    std::vector<std::pair<int, std::string>> devices;
    for (const auto& device : pa_devices) {
      devices.push_back({device.id, device.name});
    }
    return devices;
  });

  m.def("get_audio_in_devices_", []() {
    std::vector<portaudio::Device> pa_devices;
    auto status = portaudio::GetAudioInDevices(&pa_devices);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get audio input devices: " << status;
      return std::vector<std::pair<int, std::string>>();
    }
    
    std::vector<std::pair<int, std::string>> devices;
    for (const auto& device : pa_devices) {
      devices.push_back({device.id, device.name});
    }
    return devices;
  });

  m.def("get_midi_out_devices_", []() {
    std::vector<std::pair<int, std::string>> devices;
    auto status = inst::MidiExt::GetMidiDevices(&devices);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to get midi output devices: " << status;
      return std::vector<std::pair<int, std::string>>();
    }
    return devices;
  });

  m.def("exec_session_", [](const std::string& session_name) {
    LOG(INFO) << "Executing session: " << session_name;

    if (::chdir(session_name.c_str()) != 0) {
      LOG(ERROR) << "Unable to chdir into session directory: "
                 << strerror(errno);
      return false;
    }

    char* const args[5] = {
        const_cast<char*>("soir-core"),
        const_cast<char*>("--log_to_file=false"),
        const_cast<char*>("--mode=standalone"),
        const_cast<char*>("--config=etc/config.yaml"),
        nullptr,
    };

    ::execvp("soir-core", args);

    LOG(ERROR) << "Failed to exec session: " << strerror(errno);
    return false;
  });

  m.def("set_force_kill_at_shutdown_",
        [](bool force) { gRt_->SetForceKillAtShutdown(force); });
}

}  // namespace rt
}  // namespace soir
