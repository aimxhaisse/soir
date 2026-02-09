#pragma once

#include <absl/status/status.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <optional>
#include <thread>

#include "audio/audio_output.hh"
#include "audio/audio_recorder.hh"
#include "core/common.hh"
#include "core/controls.hh"
#include "core/level_meter.hh"
#include "core/sample_manager.hh"
#include "core/track.hh"
#include "utils/config.hh"

namespace soir {
namespace vst {
class VstHost;
}  // namespace vst

// First implementation is stupid and does not take
// into account lag. The timing precision of MIDI
// events is capped to a block size, we'll late see
// how to achieve intra-block precision
class Engine {
 public:
  Engine();
  ~Engine();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  void RegisterConsumer(SampleConsumer* consumer);
  void RemoveConsumer(SampleConsumer* consumer);
  void PushMidiEvent(const MidiEventAt& event);

  absl::Status SetupTracks(const std::list<Track::Settings>& settings);
  absl::Status GetTracks(std::list<Track::Settings>* settings);

  Controls* GetControls();
  vst::VstHost* GetVstHost();

  SampleManager& GetSampleManager();

  Levels GetMasterLevels() const;
  std::optional<Levels> GetTrackLevels(const std::string& name);

  absl::Status OpenVstEditor(const std::string& track_name,
                             const std::string& fx_name);
  absl::Status CloseVstEditor(const std::string& track_name,
                              const std::string& fx_name);

  absl::Status StartRecording(const std::string& file_path);
  absl::Status StopRecording();

 private:
  absl::Status Run();
  void SetTicks(std::list<MidiEventAt>& events);

  // Helper to print some statistics about CPU usage.
  void Stats(const absl::Time& next_block_at,
             const absl::Duration& block_duration) const;

  SampleTick current_tick_;

  // The main thread of the DSP engine, processes blocks of audio
  // samples in an infinite loop.
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;

  // Consumers can be registered at start if the audio output is
  // enabled. They are fed with audio samples from the DSP engine.
  std::mutex consumers_mutex_;
  bool audio_output_enabled_ = false;
  std::unique_ptr<audio::AudioOutput> audio_output_;
  std::unique_ptr<AudioRecorder> audio_recorder_;
  std::list<SampleConsumer*> consumers_;

  // Tracks are created/updated by the Runtime engine, and locked
  // during the processing of a block.
  std::mutex setup_tracks_mutex_;
  std::mutex tracks_mutex_;
  std::map<std::string, std::unique_ptr<Track>> tracks_;
  std::unique_ptr<Controls> controls_;

  // MIDI events are pushed by the RT engine and consumed by the DSP
  // engine upon each block processing at the beginning.
  std::mutex msgs_mutex_;
  std::map<std::string, std::list<MidiEventAt>> msgs_by_track_;

  std::unique_ptr<SampleManager> sample_manager_;
  std::unique_ptr<vst::VstHost> vst_host_;
  LevelMeter master_meter_;
};

}  // namespace soir
