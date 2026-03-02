#include "audio/audio_stream.hh"

#include <algorithm>
#include <chrono>

#include "absl/log/log.h"
#include "audio/audio_buffer.hh"
#include "audio/ogg_opus_encoder.hh"

namespace soir {
namespace audio {

AudioStream::AudioStream() = default;

AudioStream::~AudioStream() = default;

absl::Status AudioStream::Init(int sample_rate, int channels, int bitrate) {
  encoder_ = std::make_unique<OggOpusEncoder>();
  auto status = encoder_->Init(sample_rate, channels, bitrate);
  if (!status.ok()) {
    return status;
  }

  channels_ = channels;

  // 2MB ring buffer (~2 minutes of 128kbps Opus).
  ring_capacity_ = 2 * 1024 * 1024;
  ring_buffer_.resize(ring_capacity_);
  write_pos_ = 0;

  // Seed the ring buffer with Ogg headers so late-joining clients
  // can start from the beginning of valid Ogg data.
  const auto& headers = encoder_->GetHeaderPages();
  WriteToRingBuffer(headers);

  accumulator_.reserve(frame_size_ * channels_);

  initialized_ = true;

  LOG(INFO) << "Audio stream initialized: " << sample_rate << "Hz, " << channels
            << " channels, " << bitrate << " bps";

  return absl::OkStatus();
}

absl::Status AudioStream::PushAudioBuffer(AudioBuffer& buffer) {
  if (!initialized_) {
    return absl::OkStatus();
  }

  auto size = buffer.Size();
  if (size == 0) {
    return absl::OkStatus();
  }

  // Interleave stereo samples into the accumulator.
  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);
  for (size_t i = 0; i < size; i++) {
    accumulator_.push_back(left[i]);
    accumulator_.push_back(right[i]);
  }

  // Encode complete frames.
  size_t samples_per_frame = static_cast<size_t>(frame_size_ * channels_);
  while (accumulator_.size() >= samples_per_frame) {
    std::vector<uint8_t> encoded;
    auto status = encoder_->Encode(accumulator_.data(), frame_size_, &encoded);
    if (!status.ok()) {
      LOG(WARNING) << "Failed to encode audio frame: " << status;
      break;
    }

    if (!encoded.empty()) {
      WriteToRingBuffer(encoded);
    }

    accumulator_.erase(accumulator_.begin(),
                       accumulator_.begin() + samples_per_frame);
  }

  return absl::OkStatus();
}

void AudioStream::WriteToRingBuffer(const std::vector<uint8_t>& data) {
  std::lock_guard<std::mutex> lock(buffer_mutex_);

  for (size_t i = 0; i < data.size(); i++) {
    ring_buffer_[(write_pos_ + i) % ring_capacity_] = data[i];
  }
  write_pos_ += data.size();

  buffer_cv_.notify_all();
}

std::vector<uint8_t> AudioStream::GetHeaderPages() const {
  if (!encoder_) {
    return {};
  }
  return encoder_->GetHeaderPages();
}

AudioStream::ReadResult AudioStream::Read(size_t offset, int timeout_ms) const {
  std::unique_lock<std::mutex> lock(buffer_mutex_);

  // If caller is caught up, wait for new data.
  if (offset >= write_pos_) {
    buffer_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this, offset]() { return offset < write_pos_; });
  }

  // Still nothing after wait.
  if (offset >= write_pos_) {
    return {{}, offset};
  }

  // If offset fell behind the ring buffer, skip forward.
  size_t earliest =
      (write_pos_ > ring_capacity_) ? (write_pos_ - ring_capacity_) : 0;
  if (offset < earliest) {
    offset = earliest;
  }

  size_t available = write_pos_ - offset;

  // Cap read size to 64KB to avoid huge allocations.
  constexpr size_t kMaxRead = 64 * 1024;
  if (available > kMaxRead) {
    available = kMaxRead;
  }

  std::vector<uint8_t> data(available);
  for (size_t i = 0; i < available; i++) {
    data[i] = ring_buffer_[(offset + i) % ring_capacity_];
  }

  return {std::move(data), offset + available};
}

}  // namespace audio
}  // namespace soir
