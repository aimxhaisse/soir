#include <absl/log/log.h>
#include <algorithm>

#include "audio_output.hh"

namespace soir {

namespace {

void ListAudioDevices() {
  int num_devices = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num_devices);
  for (int i = 0; i < num_devices; i++) {
    LOG(INFO) << "Audio device [" << i
              << "]: " << SDL_GetAudioDeviceName(devices[i]);
  }
  SDL_free(devices);
}

}  // namespace

absl::Status AudioOutput::GetAudioDevices(
    std::vector<std::pair<int, std::string>>* out) {
  int num = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num);
  for (int i = 0; i < num; i++) {
    out->push_back({i, SDL_GetAudioDeviceName(devices[i])});
  }
  SDL_free(devices);
  return absl::OkStatus();
}

AudioOutput::AudioOutput() {
  SDL_Init(SDL_INIT_AUDIO);

  ListAudioDevices();
}

AudioOutput::~AudioOutput() {
  if (audio_stream_) {
    SDL_DestroyAudioStream(audio_stream_);
    audio_stream_ = nullptr;
  }
}

absl::Status AudioOutput::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing audio output";

  int num_devices = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num_devices);

  if (num_devices <= 0) {
    SDL_free(devices);
    LOG(ERROR) << "No audio devices available";
    return absl::InternalError("No audio devices available");
  }

  auto device_index = config.Get<int>("soir.dsp.output.audio.device_id", 0);
  if (device_index >= num_devices) {
    SDL_free(devices);
    LOG(ERROR) << "Invalid audio device index: " << device_index;
    return absl::InternalError("Invalid audio device index");
  }

  SDL_AudioDeviceID device_id = devices[device_index];
  auto device_name = SDL_GetAudioDeviceName(device_id);
  SDL_free(devices);

  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = kSampleRate;
  spec.format = SDL_AUDIO_F32LE;
  spec.channels = kNumChannels;

  LOG(INFO) << "Opening audio device: " << device_name << " (ID: " << device_id
            << ")"
            << " with sample rate: " << spec.freq
            << ", format: " << SDL_GetAudioFormatName(spec.format)
            << ", channels: " << static_cast<int>(spec.channels);

  audio_stream_ = SDL_OpenAudioDeviceStream(device_id, &spec, nullptr, nullptr);
  if (!audio_stream_) {
    LOG(ERROR) << "Failed to open audio device stream: " << SDL_GetError();
    return absl::InternalError("Failed to open audio device stream");
  }

  device_id_ = SDL_GetAudioStreamDevice(audio_stream_);

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  LOG(INFO) << "Starting audio output";

  if (!SDL_ResumeAudioDevice(device_id_)) {
    LOG(ERROR) << "Failed to resume audio device: " << SDL_GetError();
    return absl::InternalError("Failed to resume audio device");
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  LOG(INFO) << "Stopping audio output";

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

  std::vector<float> interleaved_buffer(size * kNumChannels);
  for (size_t i = 0; i < size; i++) {
    for (size_t j = 0; j < kNumChannels; j++) {
      interleaved_buffer[i * kNumChannels + j] = buffer.GetChannel(j)[i];
    }
  }

  if (!SDL_PutAudioStreamData(audio_stream_, interleaved_buffer.data(),
                              interleaved_buffer.size() * sizeof(float))) {
    LOG(ERROR) << "Failed to queue audio data: " << SDL_GetError();
    return absl::InternalError("Failed to queue audio data");
  }

  return absl::OkStatus();
}

}  // namespace soir
