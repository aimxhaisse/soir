#pragma once

#include <condition_variable>
#include <mutex>

#include "core/dsp/engine.hh"
#include "core/rt/notifier.hh"
#include "utils/config.hh"

namespace neon {
namespace rt {

using CbFunc = std::function<void()>;
using MicroBeat = uint64_t;

static constexpr uint64_t OneBeat = 1000000;

// Scheduled callback at a given beat.
struct Cb {
  // We use micro-beat units to prevent any precision loss with floats
  // which would lead to time drifts. We don't store any time here
  // because BPM can evolve at any moment, affecting the overall
  // scheduling.
  MicroBeat at;
  CbFunc func;

  bool operator()(const Cb& a, const Cb& b) const { return a.at <= b.at; }
};

// This is the main engine that runs the Python code and schedules
// callbacks. It uses a temporal recursion pattern to avoid time
// drifts (this is heavily inspired from Extempore).
//
// Threading model:
//
// - all Python is executed from the running loop
// - all callbacks are executed from the running loop
// - code updates are pushed from an external thread via UpdateCode()
//
// This allows callbacks to interact with Python without GIL issues.
//
// We'll need to update this model in case we want to schedule
// callbacks from other threads.
class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const utils::Config& config, dsp::Engine* dsp,
                    Notifier* notifier);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

  // This is called from another thread to evaluate a piece of Python
  // code coming from clients. Code is queued to be executed from the
  // Run() loop.
  absl::Status PushCodeUpdate(const std::string& code);

  absl::Time MicroBeatToTime(MicroBeat beat) const;
  uint64_t MicroBeatToBeat(MicroBeat beat) const;
  void Schedule(MicroBeat at, const CbFunc& func);

  // Those are part of the Live module and can be called from Python.
  float SetBPM(float bpm);
  float GetBPM() const;
  MicroBeat GetCurrentBeat() const;
  void Log(const std::string& message);
  void Beat();
  void MidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void MidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
  void MidiCC(uint8_t channel, uint8_t cc, uint8_t value);
  void MidiSysex(uint8_t channel,
                 proto::MidiSysexInstruction::InstructionType instruction,
                 const std::string& payload);

 private:
  std::thread thread_;

  Notifier* notifier_;
  dsp::Engine* dsp_;

  // Updated by the Python thread only.
  std::set<Cb, Cb> schedule_;

  // Updated by the main thread/gRPC threads.
  std::mutex loop_mutex_;
  std::condition_variable loop_cv_;
  std::list<std::string> code_updates_;
  bool running_ = false;

  // Lockless as only accessed from the Python thread.
  MicroBeat current_beat_ = 0;
  absl::Time current_time_;
  float bpm_ = 120.0;
  uint64_t beat_us_ = 0;
};

}  // namespace rt
}  // namespace neon
