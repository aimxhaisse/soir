#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "absl/status/status.h"
#include "core/common.hh"

namespace soir {
namespace audio {

class OggOpusEncoder;

class AudioStream : public SampleConsumer {
 public:
  AudioStream();
  ~AudioStream();

  // Initialize the stream encoder and ring buffer.
  absl::Status Init(int sample_rate, int channels, int bitrate = 128000);

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

  // Get the Ogg/Opus header bytes that must be sent first to each client.
  std::vector<uint8_t> GetHeaderPages() const;

  // Get the current write position for live-stream clients to start from.
  size_t GetCurrentOffset() const;

  // Read encoded audio data from the ring buffer starting at offset.
  // Blocks up to timeout_ms if no new data is available.
  struct ReadResult {
    std::vector<uint8_t> data;
    size_t new_offset;
  };

  ReadResult Read(size_t offset, int timeout_ms = 1000) const;

 private:
  void WriteToRingBuffer(const std::vector<uint8_t>& data);

  std::unique_ptr<OggOpusEncoder> encoder_;
  bool initialized_ = false;

  // Ring buffer for encoded audio data.
  mutable std::mutex buffer_mutex_;
  mutable std::condition_variable buffer_cv_;
  std::vector<uint8_t> ring_buffer_;
  size_t ring_capacity_ = 0;
  size_t write_pos_ = 0;

  // Accumulator for building complete Opus frames.
  std::vector<float> accumulator_;
  int channels_ = 0;
  int frame_size_ = 960;
};

}  // namespace audio
}  // namespace soir
