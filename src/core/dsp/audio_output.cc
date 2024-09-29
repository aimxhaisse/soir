#include <absl/log/log.h>
#include <algorithm>

#include "audio_output.hh"

namespace neon {
namespace dsp {

AudioOutput::AudioOutput() {
  SDL_Init(SDL_INIT_AUDIO);
}

AudioOutput::~AudioOutput() {}

absl::Status AudioOutput::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing audio output";

  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = kSampleRate;
  spec.format = AUDIO_F32LSB;
  spec.channels = kNumChannels;
  spec.samples = config.Get<unsigned int>("neon.dsp.engine.block_size");

  SDL_AudioSpec obtained;
  device_id_ = SDL_OpenAudioDevice(NULL, 0, &spec, &obtained, 0);
  if (device_id_ == 0) {
    LOG(ERROR) << "Failed to open audio device: " << SDL_GetError();
    return absl::InternalError("Failed to open audio device");
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  LOG(INFO) << "Starting audio output";

  SDL_PauseAudioDevice(device_id_, 0);

  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  LOG(INFO) << "Stopping audio output";

  SDL_PauseAudioDevice(device_id_, 1);

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

  SDL_QueueAudio(device_id_, interleaved_buffer.data(),
                 interleaved_buffer.size() * sizeof(float));

  return absl::OkStatus();
}

}  // namespace dsp
}  // namespace neon
