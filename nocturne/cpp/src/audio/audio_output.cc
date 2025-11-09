#include "audio/audio_output.hh"

#define MINIAUDIO_IMPLEMENTATION
#include "absl/log/log.h"
#include "miniaudio.h"

namespace soir {
namespace audio {

static void data_callback(ma_device* device, void* output, const void* input,
                          ma_uint32 frame_count) {
  (void)input;

  auto* audio_output = static_cast<AudioOutput*>(device->pUserData);
  if (audio_output && audio_output->callback_) {
    audio_output->callback_(static_cast<float*>(output), frame_count);
  } else {
    // No callback, output silence
    memset(output, 0, frame_count * device->playback.channels * sizeof(float));
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

void AudioOutput::SetCallback(AudioCallback callback) {
  callback_ = std::move(callback);
}

}  // namespace audio
}  // namespace soir
