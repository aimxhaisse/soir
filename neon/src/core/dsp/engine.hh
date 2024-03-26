#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "http.hh"
#include "track.hh"
#include "utils/config.hh"
#include "utils/misc.hh"

namespace neon {
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
  void PushMidiEvent(const libremidi::message& msg);

  absl::Status SetupTracks(const std::list<TrackSettings>& settings);
  absl::Status GetTracks(std::list<TrackSettings>* settings);

 private:
  absl::Status Run();

  // Helper to print some statistics about CPU usage.
  void Stats(const absl::Time& next_block_at,
             const absl::Duration& block_duration) const;

  uint32_t block_size_;

  // The main thread of the DSP engine, processes blocks of audio
  // samples in an infinite loop.
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_ = false;
  std::unique_ptr<HttpServer> http_server_;

  // Consumers are registered by the HTTP server upon new connections
  // and fed with audio samples by the DSP engine.
  std::mutex consumers_mutex_;
  std::list<SampleConsumer*> consumers_;

  // Tracks are created/updated by the Runtime engine, and locked
  // during the processing of a block.
  std::mutex setup_tracks_mutex_;
  std::mutex tracks_mutex_;
  std::map<int, std::unique_ptr<Track>> tracks_;

  // MIDI events are pushed by the Soir engine and consumed by the DSP
  // engine upon each block processing at the beginning.
  std::mutex msgs_mutex_;
  std::map<int, std::list<libremidi::message>> msgs_by_chan_;
};

}  // namespace dsp
}  // namespace neon
