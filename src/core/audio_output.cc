#include <absl/log/log.h>
#include <algorithm>
#include <cstring>

#include "audio_output.hh"
#include "portaudio.hh"

namespace soir {

void AudioOutput::PaStreamDeleter::operator()(PaStream* stream) {
  if (stream) {
    Pa_CloseStream(stream);
  }
}

AudioOutput::AudioOutput() {
  auto status = portaudio::Initialize();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize PortAudio: " << status;
    return;
  }

  portaudio::ListAudioOutDevices();
  
  // Initialize ring buffer (4x block size for buffering)
  ring_buffer_size_ = kBlockSize * kNumChannels * 4;
  ring_buffer_.resize(ring_buffer_size_);
}

AudioOutput::~AudioOutput() {
  Stop();
  stream_.reset();
  portaudio::Terminate();
}

int AudioOutput::AudioCallback(const void* in, void* out,
                              unsigned long frames,
                              const PaStreamCallbackTimeInfo* time,
                              PaStreamCallbackFlags flags,
                              void* user_data) {
  auto* audio_output = static_cast<AudioOutput*>(user_data);
  auto* output = static_cast<float*>(out);
  
  std::lock_guard<std::mutex> lock(audio_output->buffer_mutex_);
  
  size_t samples_needed = frames * kNumChannels;
  size_t samples_available = 0;
  
  if (audio_output->ring_buffer_write_pos_ >= audio_output->ring_buffer_read_pos_) {
    samples_available = audio_output->ring_buffer_write_pos_ - audio_output->ring_buffer_read_pos_;
  } else {
    samples_available = audio_output->ring_buffer_size_ - audio_output->ring_buffer_read_pos_ + audio_output->ring_buffer_write_pos_;
  }
  
  if (samples_available >= samples_needed) {
    for (size_t i = 0; i < samples_needed; ++i) {
      output[i] = audio_output->ring_buffer_[audio_output->ring_buffer_read_pos_];
      audio_output->ring_buffer_read_pos_ = (audio_output->ring_buffer_read_pos_ + 1) % audio_output->ring_buffer_size_;
    }
  } else {
    // Not enough data, output silence
    std::memset(output, 0, samples_needed * sizeof(float));
  }
  
  return paContinue;
}

absl::Status AudioOutput::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing audio output";

  std::vector<portaudio::Device> devices;
  auto status = portaudio::GetAudioOutDevices(&devices);
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

  const auto& device = devices[device_index];

  LOG(INFO) << "Opening audio device: " << device.name << " (ID: " << device.id
            << ") with sample rate: " << kSampleRate
            << ", channels: " << kNumChannels;

  PaStreamParameters outputParameters;
  outputParameters.device = device.id;
  outputParameters.channelCount = kNumChannels;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(device.id)->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = nullptr;

  PaStream* raw_stream = nullptr;
  PaError err = Pa_OpenStream(&raw_stream,
                             nullptr,  // no input
                             &outputParameters,
                             kSampleRate,
                             kBlockSize,
                             paNoFlag,
                             AudioCallback,
                             this);

  if (err != paNoError) {
    LOG(ERROR) << "Failed to open audio stream: " << Pa_GetErrorText(err);
    return absl::InternalError("Failed to open audio stream");
  }

  stream_.reset(raw_stream);
  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  LOG(INFO) << "Starting audio output";

  if (!stream_) {
    return absl::InternalError("Audio stream not initialized");
  }

  PaError err = Pa_StartStream(stream_.get());
  if (err != paNoError) {
    LOG(ERROR) << "Failed to start audio stream: " << Pa_GetErrorText(err);
    return absl::InternalError("Failed to start audio stream");
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  LOG(INFO) << "Stopping audio output";

  if (!stream_) {
    return absl::OkStatus();
  }

  PaError err = Pa_StopStream(stream_.get());
  if (err != paNoError) {
    LOG(ERROR) << "Failed to stop audio stream: " << Pa_GetErrorText(err);
    return absl::InternalError("Failed to stop audio stream");
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::PushAudioBuffer(AudioBuffer& buffer) {
  auto size = buffer.Size();

  if (size == 0) {
    return absl::OkStatus();
  }

  std::lock_guard<std::mutex> lock(buffer_mutex_);

  // Interleave the audio data
  for (size_t i = 0; i < size; i++) {
    for (size_t j = 0; j < kNumChannels; j++) {
      ring_buffer_[ring_buffer_write_pos_] = buffer.GetChannel(j)[i];
      ring_buffer_write_pos_ = (ring_buffer_write_pos_ + 1) % ring_buffer_size_;
    }
  }

  return absl::OkStatus();
}

}  // namespace soir
