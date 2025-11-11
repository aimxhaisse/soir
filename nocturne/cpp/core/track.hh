#pragma once

#include <absl/status/status.h>

#include <atomic>
#include <condition_variable>
#include <libremidi/libremidi.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

#include "audio/audio_buffer.hh"
#include "core/common.hh"
#include "core/controls.hh"
#include "core/midi_stack.hh"
#include "core/parameter.hh"
#include "core/sample_manager.hh"
#include "fx/fx_stack.hh"
#include "inst/instrument.hh"
#include "inst/midi_ext.hh"
#include "inst/sampler.hh"
#include "utils/config.hh"

namespace soir {

// Only sample tracks for now, keep it stupid simple
// before we introduce more complex stuff.
struct Track {
  // Settings of a track.
  struct Settings {
    std::string name_ = "unknown";
    inst::Type instrument_ = inst::Type::UNKNOWN;
    bool muted_ = false;
    Parameter volume_;
    Parameter pan_;
    std::string extra_;
    std::list<fx::Fx::Settings> fxs_;
  };

  Track();
  ~Track();

  absl::Status Init(const Settings& settings, SampleManager* sample_manager,
                    Controls* controls);
  absl::Status Start();
  absl::Status Stop();

  // If MaybeFastUpdate returns false, it means the track can't update
  // itself quickly so it likely needs to be re-created.
  bool CanFastUpdate(const Settings& settings);
  void FastUpdate(const Settings& settings);

  Settings GetSettings();
  const std::string& GetTrackName();

  // Schedule an async render operation
  void RenderAsync(SampleTick tick, const std::list<MidiEventAt>& events);

  // Wait for rendering to complete and mix the result into the output buffer
  void Join(AudioBuffer& output_buffer);

 private:
  // Thread function for processing audio
  absl::Status ProcessLoop();

  Controls* controls_;
  SampleManager* sample_manager_;

  std::mutex mutex_;
  Settings settings_;
  std::unique_ptr<inst::Instrument> inst_;
  std::unique_ptr<fx::FxStack> fx_stack_;
  MidiStack midi_stack_;

  // Thread management
  std::thread thread_;
  std::mutex work_mutex_;
  std::condition_variable work_cv_;
  std::condition_variable done_cv_;
  std::atomic<bool> stop_thread_ = false;
  std::atomic<bool> has_work_ = false;
  std::atomic<bool> work_done_ = false;

  // Work parameters
  SampleTick current_tick_;
  std::list<MidiEventAt> current_events_;
  AudioBuffer track_buffer_;
};

}  // namespace soir
