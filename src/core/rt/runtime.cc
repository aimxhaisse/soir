#include <absl/log/log.h>
#include <absl/time/clock.h>
#include <pybind11/embed.h>
#include <libremidi/libremidi.hpp>

#include "core/rt/bindings.hh"
#include "core/rt/runtime.hh"
#include "utils/signal.hh"

namespace py = pybind11;

namespace soir {
namespace rt {

Runtime::Runtime() : notifier_(nullptr) {}

Runtime::~Runtime() {
  bindings::ResetEngines();
}

absl::Status Runtime::Init(const utils::Config& config, Engine* dsp,
                          Notifier* notifier) {
  LOG(INFO) << "Initializing runtime";

  python_paths_ = config.Get<std::vector<std::string>>("soir.rt.python_paths");
  if (python_paths_.empty()) {
    LOG(ERROR) << "Python paths is empty";
    return absl::InvalidArgumentError("Python paths is empty");
  }
  notifier_ = notifier;
  dsp_ = dsp;
  current_time_ = absl::Now();
  SetBPM(config.Get<uint16_t>("soir.rt.initial_bpm"));

  Beat();

  auto status = bindings::SetEngines(this, dsp);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to set DSP/RT engine: " << status;
    return status;
  }

  running_ = true;

  return absl::OkStatus();
}

absl::Status Runtime::Start() {
  LOG(INFO) << "Starting runtime";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Runtime failed: " << status;
    }
  });

  return absl::OkStatus();
}

absl::Status Runtime::Stop() {
  LOG(INFO) << "Stopping runtime";

  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    running_ = false;
    loop_cv_.notify_all();
  }

  thread_.join();

  LOG(INFO) << "Runtime stopped";

  return absl::OkStatus();
}

absl::Time Runtime::MicroBeatToTime(MicroBeat beat) const {
  MicroBeat diff_mb = (beat > current_beat_) ? beat - current_beat_ : 0;
  uint64_t diff_us = (diff_mb * beat_us_) / 1000000.0;

  return current_time_ + absl::Microseconds(diff_us);
}

MicroBeat Runtime::DurationToMicroBeat(absl::Duration duration) const {
  auto duration_us = duration / absl::Microseconds(1);

  return bpm_ * duration_us / (60.0 * 1000000.0);
}

uint64_t Runtime::MicroBeatToBeat(MicroBeat beat) const {
  return beat / kOneBeat;
}

absl::Status Runtime::Run() {
  py::scoped_interpreter guard{};

  // Import soir module using the Python path provided in the config.
  // Alternatively we could do this via pyproject as well, unclear what
  // the user-packaged setup will be at this stage.
  py::module_ sys = py::module_::import("sys");
  for (auto python_path : python_paths_) {
    sys.attr("path").attr("append")(python_path);
  }
  py::module_ soir_mod = py::module_::import("soir");

  LOG(INFO) << "Python version: " << std::string(py::str(sys.attr("version")));

  // Setup the initial feedback loop for controls.
  py::exec(R"(soir._ctrls.update_loop_())", soir_mod.attr("__dict__"));

  while (true) {
    // We assume there is always at least one callback in the queue
    // due to the beat scheduling.
    auto next = schedule_.begin();
    auto at_time = Runtime::MicroBeatToTime(next->at);

    std::string code;
    {
      std::unique_lock<std::mutex> lock(loop_mutex_);

      loop_cv_.wait_until(
          lock, absl::ToChronoTime(at_time), [this, next, at_time] {
            return !running_ || !code_.empty() || at_time <= absl::Now();
          });

      if (!running_) {
        LOG(INFO) << "Received stop signal";
        break;
      }

      if (!code_.empty()) {
        std::swap(code, code_);
      }
    }

    // Process next callback if time has passed.
    if (at_time <= absl::Now()) {
      // This is set before the callback is executed so that it can
      // retrieve accurate timing information.
      current_time_ = at_time;
      current_beat_ = next->at;

      try {
        next->func();
      } catch (py::error_already_set& e) {
        if (e.matches(PyExc_SystemExit)) {
          LOG(INFO) << "Received SystemExit, stopping runtime";
          soir::utils::SignalExit();
          return absl::OkStatus();
        }
        LOG(ERROR) << "Python error: " << e.what();
      }

      schedule_.erase(next);
    }

    // Code updates are performed in a second time, after the temporal
    // recursions, to be as precise on time as possible. It's OK if a
    // code update takes 10ms to be applied, but not OK if it's a kick
    // event for example.
    if (!code.empty()) {
      auto now = absl::Now();

      // We do not update current_time_ here: this would delay all
      // subsequent callbacks by the time it took to apply the code
      // update. Current time is not available to the Python engine so
      // it's fine. Current beat is however, and we do want to keep it
      // accurate so that the Python engine can use it to schedule
      // events while padding to beat new loop creations with
      // alignment.
      current_beat_ += Runtime::DurationToMicroBeat(now - current_time_);

      try {
        // We set the last evaluated code at the last moment so that
        // inspection of code can be done only when it is actually
        // executed.
        last_evaluated_code_ = code;
        py::exec(code.c_str(), soir_mod.attr("__dict__"));

        // Maybe here we can have some sort of post-execution hook
        // that can be used to do some cleanup or other operations.
        py::exec("soir._internals.post_eval_()", soir_mod.attr("__dict__"));
        py::exec("soir._ctrls.post_eval_()", soir_mod.attr("__dict__"));
      } catch (py::error_already_set& e) {
        if (e.matches(PyExc_SystemExit)) {
          LOG(INFO) << "Received SystemExit, stopping runtime";
          soir::utils::SignalExit();
          return absl::OkStatus();
        }
        LOG(ERROR) << "Python error: " << e.what();
      }
    }
  }

  // Clear code here to explicitly delete python functions references,
  // otherwise they will be deleted when the interpreter has completed
  // causing random crashes.
  schedule_.clear();

  return absl::OkStatus();
}

float Runtime::SetBPM(float bpm) {
  LOG(INFO) << "Setting BPM to " << bpm;

  bpm_ = bpm;
  beat_us_ = 60.0 / bpm_ * 1000000.0;

  return bpm_;
}

float Runtime::GetBPM() const {
  return bpm_;
}

MicroBeat Runtime::GetCurrentBeat() const {
  return current_beat_;
}

void Runtime::Log(const std::string& message) {
  proto::GetLogsResponse note;

  note.set_notification(message);

  auto status = notifier_->Notify(note);
  if (!status.ok()) {
    LOG(WARNING) << "Unable to send log notification: " << status;
  }
}

void Runtime::Beat() {
  LOG(INFO) << "Beat " << MicroBeatToBeat(current_beat_);

  Schedule(current_beat_ + kOneBeat, [this]() { Beat(); });
}

void Runtime::MidiNoteOn(const std::string& track, uint8_t channel, uint8_t note,
                        uint8_t velocity) {
  auto message = libremidi::channel_events::note_on(channel, note, velocity);

  dsp_->PushMidiEvent(MidiEventAt(track, message, current_time_));
}

void Runtime::MidiNoteOff(const std::string& track, uint8_t channel,
                         uint8_t note, uint8_t velocity) {
  auto message = libremidi::channel_events::note_off(channel, note, velocity);

  dsp_->PushMidiEvent(MidiEventAt(track, message, current_time_));
}

void Runtime::MidiCC(const std::string& track, uint8_t channel, uint8_t cc,
                    uint8_t value) {
  auto message = libremidi::channel_events::control_change(channel, cc, value);

  dsp_->PushMidiEvent(MidiEventAt(track, message, current_time_));
}

void Runtime::MidiSysex(const std::string& track,
                       proto::MidiSysexInstruction::InstructionType instruction,
                       const std::string& json_payload) {
  proto::MidiSysexInstruction inst;

  inst.set_type(instruction);
  inst.set_json_payload(json_payload);

  std::string inst_serialized = inst.SerializeAsString();

  libremidi::midi_bytes bytes;

  bytes.push_back(static_cast<char>(libremidi::message_type::SYSTEM_EXCLUSIVE));
  bytes.insert(bytes.begin() + 1, inst_serialized.begin(),
               inst_serialized.end());

  dsp_->PushMidiEvent(
      MidiEventAt(track, libremidi::message(bytes, 0), current_time_));
}

std::string Runtime::GetCode() const {
  return last_evaluated_code_;
}

void Runtime::Schedule(MicroBeat at, const CbFunc& cb) {
  // This is stupid simple because we currently don't support
  // scheduling callbacks from multiple threads. So it is assumed here
  // we are running in the context of Run(). If we ever support
  // external scheduling, we'll need to wake up the Run loop here in
  // case the next scheduled callback changes.

  schedule_.insert({at, cb, last_cb_id_++});
}

absl::Status Runtime::PushCodeUpdate(const std::string& code) {
  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    code_ = code;
    loop_cv_.notify_all();
  }

  LOG(INFO) << "Code update queued";

  return absl::OkStatus();
}

}  // namespace rt
}  // namespace soir