#include <absl/log/log.h>
#include <algorithm>
#include <cstring>

#include "audio_output.hh"
#include "sdl.hh"

namespace soir {

AudioOutput::AudioOutput() {
  auto status = sdl::Initialize();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize SDL: " << status;
    return;
  }

  sdl::ListAudioOutDevices();
}

AudioOutput::~AudioOutput() {
  Stop();
  if (audio_stream_) {
    SDL_DestroyAudioStream(audio_stream_);
    audio_stream_ = nullptr;
  }
  sdl::Terminate();
}

void AudioOutput::AudioCallback(void* userdata, SDL_AudioStream* stream,
                                int additional_amount, int total_amount) {
  auto* audio_output = static_cast<AudioOutput*>(userdata);

  std::lock_guard<std::mutex> lock(audio_output->buffer_mutex_);

  if (!audio_output->buffer_.empty()) {
    SDL_PutAudioStreamData(stream, audio_output->buffer_.data(),
                           audio_output->buffer_.size() * sizeof(float));
    audio_output->buffer_.clear();
  }
}

absl::Status AudioOutput::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing audio output";

  std::vector<sdl::Device> devices;
  auto status = sdl::GetAudioOutDevices(&devices);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get audio devices: " << status;
    return status;
  }

  if (devices.empty()) {
    LOG(ERROR) << "No audio devices available";
    return absl::InternalError("No audio devices available");
  }

  auto device_index = config.Get<int>("soir.dsp.output.audio.device_id", 0);
  if (device_index >= static_cast<int>(devices.size())) {
    LOG(ERROR) << "Invalid audio device index: " << device_index;
    return absl::InternalError("Invalid audio device index");
  }

  // Get actual device IDs from SDL
  int num_devices = 0;
  SDL_AudioDeviceID* device_ids = SDL_GetAudioPlaybackDevices(&num_devices);
  if (!device_ids || device_index >= num_devices) {
    LOG(ERROR) << "Failed to get SDL audio devices";
    if (device_ids)
      SDL_free(device_ids);
    return absl::InternalError("Failed to get SDL audio devices");
  }

  SDL_AudioDeviceID device_id = device_ids[device_index];
  const char* device_name = SDL_GetAudioDeviceName(device_id);
  SDL_free(device_ids);

  LOG(INFO) << "Opening audio device: "
            << (device_name ? device_name : "Unknown") << " (ID: " << device_id
            << ") with sample rate: " << kSampleRate
            << ", channels: " << kNumChannels;

  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = kSampleRate;
  spec.format = SDL_AUDIO_F32LE;
  spec.channels = kNumChannels;

  audio_stream_ =
      SDL_OpenAudioDeviceStream(device_id, &spec, AudioCallback, this);
  if (!audio_stream_) {
    LOG(ERROR) << "Failed to open audio device stream: " << SDL_GetError();
    return absl::InternalError("Failed to open audio device stream");
  }

  device_id_ = SDL_GetAudioStreamDevice(audio_stream_);

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  LOG(INFO) << "Starting audio output";

  if (!audio_stream_) {
    return absl::InternalError("Audio stream not initialized");
  }

  if (!SDL_ResumeAudioDevice(device_id_)) {
    LOG(ERROR) << "Failed to resume audio device: " << SDL_GetError();
    return absl::InternalError("Failed to resume audio device");
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  LOG(INFO) << "Stopping audio output";

  if (!audio_stream_) {
    return absl::OkStatus();
  }

  if (!SDL_PauseAudioDevice(device_id_)) {
    LOG(ERROR) << "Failed to pause audio device: " << SDL_GetError();
    return absl::InternalError("Failed to pause audio device");
  }

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
    for (size_t j = 0; j < kNumChannels; j++) {
      buffer_.push_back(buffer.GetChannel(j)[i]);
    }
  }

  return absl::OkStatus();
}

}  // namespace soir
