#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "ogg/ogg.h"

struct OpusEncoder;

namespace soir {
namespace audio {

struct OpusEncoderDeleter {
  void operator()(OpusEncoder* e) const;
};

class OggOpusEncoder {
 public:
  OggOpusEncoder();
  ~OggOpusEncoder();

  // Initialize encoder. Must be called before Encode().
  // sample_rate: input sample rate (48000)
  // channels: number of channels (2)
  // bitrate: target bitrate in bits/sec (128000)
  absl::Status Init(int sample_rate, int channels, int bitrate);

  // Encode interleaved float samples. Input must be exactly
  // frame_size * channels floats. frame_size should be a valid Opus
  // frame size (120, 240, 480, 960, 1920, 2880 samples).
  // Appends resulting Ogg pages to output.
  absl::Status Encode(const float* pcm, int frame_size,
                      std::vector<uint8_t>* output);

  // Flush any remaining Ogg pages and write end-of-stream.
  absl::Status Flush(std::vector<uint8_t>* output);

  // Get the Ogg/Opus header pages (must be sent to clients first).
  const std::vector<uint8_t>& GetHeaderPages() const;

 private:
  absl::Status WriteOggPages(std::vector<uint8_t>* output, bool flush);
  absl::Status GenerateHeaders();

  std::unique_ptr<OpusEncoder, OpusEncoderDeleter> encoder_;
  ogg_stream_state ogg_stream_ = {};
  int sample_rate_ = 0;
  int channels_ = 0;
  int64_t granulepos_ = 0;
  int64_t packetno_ = 0;
  std::vector<uint8_t> header_pages_;
  bool initialized_ = false;
};

}  // namespace audio
}  // namespace soir
