#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "absl/status/status.h"
#include "core/common.hh"
#include "readerwriterqueue.h"

namespace soir {
namespace audio {

class OggOpusEncoder;

class AudioStream : public SampleConsumer {
 public:
  AudioStream();
  ~AudioStream() override;

  absl::Status Init(int sample_rate, int channels, int bitrate = 128000);

  // Stop the encoder thread. Idempotent.
  void Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  // Get the Ogg/Opus header bytes that must be sent first to each client.
  std::vector<uint8_t> GetHeaderPages() const;

  // Read encoded audio data from the ring buffer starting at offset.
  // Blocks up to timeout_ms if no new data is available.
  struct ReadResult {
    std::vector<uint8_t> data;
    size_t new_offset;
  };

  ReadResult Read(size_t offset, int timeout_ms = 1000) const;

 private:
  void WriteToRingBuffer(const std::vector<uint8_t>& data);
  void EncoderLoop();

  std::unique_ptr<OggOpusEncoder> encoder_;
  bool initialized_;

  mutable std::mutex buffer_mutex_;
  mutable std::condition_variable buffer_cv_;
  std::vector<uint8_t> ring_buffer_;
  size_t ring_capacity_;
  size_t write_pos_;

  // Engine (sole producer) -> encoder thread (sole consumer): Opus
  // encoding stays off the engine thread so it can't underrun the device.
  moodycamel::BlockingReaderWriterQueue<float> pcm_queue_;
  std::thread encoder_thread_;
  std::atomic<bool> stop_;
  int channels_;
  int frame_size_;
};

}  // namespace audio
}  // namespace soir
