#include "audio/audio_stream.hh"

#include <chrono>

#include "absl/log/log.h"
#include "audio/audio_buffer.hh"
#include "audio/ogg_opus_encoder.hh"

namespace soir {
namespace audio {

AudioStream::AudioStream()
    : initialized_(false),
      ring_capacity_(0),
      write_pos_(0),
      stop_(false),
      channels_(0),
      frame_size_(960) {}

AudioStream::~AudioStream() { Stop(); }

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

  // Seed the ring with Ogg headers so late-joining clients get valid Ogg.
  const auto& headers = encoder_->GetHeaderPages();
  WriteToRingBuffer(headers);

  stop_ = false;
  encoder_thread_ = std::thread([this]() { EncoderLoop(); });

  initialized_ = true;

  LOG(INFO) << "Audio stream initialized: " << sample_rate << "Hz, " << channels
            << " channels, " << bitrate << " bps";

  return absl::OkStatus();
}

void AudioStream::Stop() {
  if (!initialized_) {
    return;
  }

  stop_ = true;
  if (encoder_thread_.joinable()) {
    encoder_thread_.join();
  }
}

absl::Status AudioStream::PushAudioBuffer(AudioBuffer& buffer) {
  if (!initialized_) {
    return absl::OkStatus();
  }

  auto size = buffer.Size();
  if (size == 0) {
    return absl::OkStatus();
  }

  const float* left = buffer.GetChannel(kLeftChannel);
  const float* right = buffer.GetChannel(kRightChannel);
  for (size_t i = 0; i < size; i++) {
    pcm_queue_.enqueue(left[i]);
    pcm_queue_.enqueue(right[i]);
  }

  return absl::OkStatus();
}

void AudioStream::EncoderLoop() {
  // Accumulator for building complete Opus frames. Encoder thread only.
  std::vector<float> pending;
  pending.reserve(static_cast<size_t>(frame_size_) * channels_);

  const size_t frame_samples = static_cast<size_t>(frame_size_) * channels_;

  while (true) {
    float sample;
    if (!pcm_queue_.wait_dequeue_timed(sample, 1000)) {
      if (stop_) {
        break;
      }
      continue;
    }
    pending.push_back(sample);

    while (pending.size() >= frame_samples) {
      std::vector<uint8_t> encoded;
      auto status = encoder_->Encode(pending.data(), frame_size_, &encoded);
      if (!status.ok()) {
        // Skip the frame rather than retrying it forever on this thread.
        LOG_EVERY_N_SEC(WARNING, 1)
            << "Failed to encode audio frame: " << status;
      } else if (!encoded.empty()) {
        WriteToRingBuffer(encoded);
      }
      pending.erase(pending.begin(), pending.begin() + frame_samples);
    }
  }
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

  if (offset >= write_pos_) {
    buffer_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this, offset]() { return offset < write_pos_; });
  }

  if (offset >= write_pos_) {
    return {{}, offset};
  }

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
