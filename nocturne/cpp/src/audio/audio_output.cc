#include "audio/audio_output.hh"

#define MINIAUDIO_IMPLEMENTATION

#include "absl/log/log.h"
#include "audio/audio_buffer.hh"
#include "miniaudio.h"

namespace soir {
namespace audio {

static void data_callback(ma_device* device, void* output, const void* input,
                          ma_uint32 frame_count) {
  (void)input;

  auto* audio_output = static_cast<AudioOutput*>(device->pUserData);
  float* output_buffer = static_cast<float*>(output);
  ma_uint32 samples_needed = frame_count * device->playback.channels;

  if (audio_output) {
    std::lock_guard<std::mutex> lock(audio_output->buffer_mutex_);

    // Consume from buffer if available
    size_t available = audio_output->audio_buffer_.size();
    size_t to_copy = std::min(static_cast<size_t>(samples_needed), available);

    if (to_copy > 0) {
      memcpy(output_buffer, audio_output->audio_buffer_.data(),
             to_copy * sizeof(float));
      audio_output->audio_buffer_.erase(
          audio_output->audio_buffer_.begin(),
          audio_output->audio_buffer_.begin() + to_copy);
    }

    // Fill remainder with silence if buffer underrun
    if (to_copy < samples_needed) {
      memset(output_buffer + to_copy, 0,
             (samples_needed - to_copy) * sizeof(float));
    }
  } else {
    // No audio_output, output silence
    memset(output_buffer, 0, samples_needed * sizeof(float));
  }
}

AudioOutput::AudioOutput() { device_ = new ma_device(); }

AudioOutput::~AudioOutput() {
  if (initialized_) {
    ma_device_uninit(device_);
  }
  delete device_;
}

absl::Status AudioOutput::Init(int sample_rate, int channels, int buffer_size) {
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = channels;
  config.sampleRate = sample_rate;
  config.dataCallback = data_callback;
  config.pUserData = this;
  config.periodSizeInFrames = buffer_size;

  if (ma_device_init(NULL, &config, device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to initialize audio device");
  }

  initialized_ = true;
  LOG(INFO) << "Audio output initialized: " << sample_rate << "Hz, " << channels
            << " channels, " << buffer_size << " frames";

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Audio output not initialized");
  }

  if (ma_device_start(device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to start audio device");
  }

  LOG(INFO) << "Audio output started";
  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Audio output not initialized");
  }

  if (ma_device_stop(device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to stop audio device");
  }

  LOG(INFO) << "Audio output stopped";
  return absl::OkStatus();
}

absl::Status AudioOutput::PushAudioBuffer(AudioBuffer& buffer) {
  auto size = buffer.Size();
  if (size == 0) {
    return absl::OkStatus();
  }

  std::lock_guard<std::mutex> lock(buffer_mutex_);

  // Interleave the audio data and append to buffer
  for (size_t i = 0; i < size; i++) {
    audio_buffer_.push_back(buffer.GetChannel(kLeftChannel)[i]);
    audio_buffer_.push_back(buffer.GetChannel(kRightChannel)[i]);
  }

  return absl::OkStatus();
}

}  // namespace audio
}  // namespace soir
