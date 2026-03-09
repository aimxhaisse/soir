#include "audio/pcm_stream.hh"

#include <absl/log/log.h>

#include <chrono>

#include "audio/audio_buffer.hh"

namespace soir {
namespace audio {

PcmStream::PcmStream() = default;
PcmStream::~PcmStream() = default;

absl::Status PcmStream::Init() {
  // ~10 seconds of stereo float32 at 48kHz.
  ring_capacity_ = kSampleRate * kNumChannels * 10;
  ring_buffer_.resize(ring_capacity_);
  write_pos_ = 0;
  initialized_ = true;
  LOG(INFO) << "PCM stream initialized";
  return absl::OkStatus();
}

absl::Status PcmStream::PushAudioBuffer(AudioBuffer& buffer) {
  if (!initialized_) {
    return absl::OkStatus();
  }

  auto size = buffer.Size();
  if (size == 0) {
    return absl::OkStatus();
  }

  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);

  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    for (size_t i = 0; i < size; i++) {
      ring_buffer_[(write_pos_ + i * 2) % ring_capacity_] = left[i];
      ring_buffer_[(write_pos_ + i * 2 + 1) % ring_capacity_] = right[i];
    }
    write_pos_ += size * 2;
  }
  buffer_cv_.notify_all();

  return absl::OkStatus();
}

size_t PcmStream::GetCurrentOffset() const {
  std::lock_guard<std::mutex> lock(buffer_mutex_);
  return write_pos_;
}

PcmStream::ReadResult PcmStream::Read(size_t offset) const {
  std::unique_lock<std::mutex> lock(buffer_mutex_);
  buffer_cv_.wait_for(lock, std::chrono::milliseconds(50),
                      [this, offset]() { return offset < write_pos_; });

  if (offset >= write_pos_) {
    return {{}, offset};
  }

  size_t earliest =
      (write_pos_ > ring_capacity_) ? (write_pos_ - ring_capacity_) : 0;
  if (offset < earliest) {
    offset = earliest;
  }

  size_t available = write_pos_ - offset;
  // Cap to one block so we deliver data at the same cadence the engine
  // produces it rather than sending large catch-up bursts.
  constexpr size_t kMaxRead = kBlockSize * kNumChannels;
  if (available > kMaxRead) {
    available = kMaxRead;
  }

  std::vector<float> data(available);
  for (size_t i = 0; i < available; i++) {
    data[i] = ring_buffer_[(offset + i) % ring_capacity_];
  }

  return {std::move(data), offset + available};
}

}  // namespace audio
}  // namespace soir
