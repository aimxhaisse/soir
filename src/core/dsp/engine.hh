#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "core/common.hh"
#include "core/dsp/audio_output.hh"
#include "core/dsp/http.hh"
#include "core/dsp/sample_manager.hh"
#include "core/dsp/track.hh"
#include "utils/config.hh"
#include "utils/misc.hh"

namespace soir {
namespace dsp {

class HttpServer;

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

  SampleManager& GetSampleManager();

 private:
  absl::Status Run();
  void SetTicks(std::list<MidiEventAt>& events);

  // Helper to print some statistics about CPU usage.
  void Stats(const absl::Time& next_block_at,
             const absl::Duration& block_duration) const;

  MicroBeat current_tick_;

  // The main thread of the DSP engine, processes blocks of audio
  // samples in an infinite loop.
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;

  // Consumers can be registered at start if the audio output is
  // enabled or by the HTTP server upon new connections. They are fed
  // with audio samples from the DSP engine.
  std::mutex consumers_mutex_;
  std::unique_ptr<HttpServer> http_server_;
  std::unique_ptr<AudioOutput> audio_output_;
  std::list<SampleConsumer*> consumers_;

  // Tracks are created/updated by the Runtime engine, and locked
  // during the processing of a block.
  std::mutex setup_tracks_mutex_;
  std::mutex tracks_mutex_;
  std::map<std::string, std::unique_ptr<Track>> tracks_;

  // MIDI events are pushed by the RT engine and consumed by the DSP
  // engine upon each block processing at the beginning.
  std::mutex msgs_mutex_;
  std::map<std::string, std::list<MidiEventAt>> msgs_by_track_;

  std::unique_ptr<SampleManager> sample_manager_;
};

}  // namespace dsp
}  // namespace soir
