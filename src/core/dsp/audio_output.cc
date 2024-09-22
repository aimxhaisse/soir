#include <absl/log/log.h>
#include <algorithm>

#include "audio_output.hh"

namespace neon {
namespace dsp {

AudioOutput::AudioOutput() : current_(0) {}

AudioOutput::~AudioOutput() {}

absl::Status AudioOutput::Init(const utils::Config& config) {
  LOG(INFO) << "Initializing audio output";

  buffer_size_ = config.Get<unsigned int>("neon.dsp.engine.block_size");

  audio_ = std::make_unique<RtAudio>();

  if (audio_->getDeviceCount() < 1) {
    return absl::InternalError("No audio devices found");
  }

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED & ~RTAUDIO_HOG_DEVICE;
  options.numberOfBuffers = kNumBuffers;
  options.priority = 99;

  RtAudio::StreamParameters params;
  params.deviceId = audio_->getDefaultOutputDevice();
  params.nChannels = kNumChannels;

  auto cb = [](void* out, void*, unsigned int frames, double,
               RtAudioStreamStatus, void* user_data) {
    auto status = static_cast<AudioOutput*>(user_data)->Consume(
        reinterpret_cast<float*>(out), frames);

    if (!status.ok()) {
      LOG(ERROR) << "Failed to feed audio buffer: " << status;
      return 1;
    }

    return 0;
  };

  int err = audio_->openStream(&params, nullptr, RTAUDIO_FLOAT32, kSampleRate,
                               &buffer_size_, cb, this, &options);
  if (err != 0) {
    return absl::InternalError("Failed to open audio stream, error= " +
                               std::to_string(err));
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  LOG(INFO) << "Starting audio output";

  int err = audio_->startStream();
  if (err != 0) {
    return absl::InternalError("Failed to start audio stream, error= " +
                               std::to_string(err));
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  LOG(INFO) << "Stopping audio output";

  if (audio_->isStreamRunning()) {
    audio_->stopStream();
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cond_.notify_one();
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::Consume(float* out, unsigned int frames) {
  int n = 0;

  float* out_left = out;
  float* out_right = out + frames;

  while (n != frames) {

    // Consume from the existing buffer if there are samples left. We
    // don't need to take any lock so it's fast. We use
    // non-interleaved format so we can bulk-copy all samples at once.
    while (position_ != current_.Size()) {
      unsigned int take = std::min(static_cast<std::size_t>(frames - n),
                                   current_.Size() - position_);

      float* left = current_.GetChannel(0);
      float* right = current_.GetChannel(1);

      memcpy(out_left + n, left + position_, take * sizeof(float));
      memcpy(out_right + n, right + position_, take * sizeof(float));

      position_ += take;
      n += take;

      if (position_ == current_.Size()) {
        position_ = 0;
        current_ = AudioBuffer(0);
      }

      if (n == frames) {
        return absl::OkStatus();
      }
    }

    // Pick one buffer from the consumed stream, we could be smarter
    // here and maybe take multiple ones if we have a lot of frames to
    // consume, however we have in most cases some synchronicity here
    // and have the same buffer size on both sides.
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cond_.wait(lock, [this]() { return !stream_.empty() || stop_; });
      if (stop_) {
        LOG(INFO) << "Audio output thread stopped";
        return absl::InternalError("Audio output thread stopped");
      }

      current_ = stream_.front();
      position_ = 0;
      stream_.pop_front();
    }
  }

  return absl::OkStatus();
}

absl::Status AudioOutput::PushAudioBuffer(const AudioBuffer& buffer) {
  std::unique_lock<std::mutex> lock(mutex_);

  stream_.push_back(buffer);
  cond_.notify_one();

  return absl::OkStatus();
}

}  // namespace dsp
}  // namespace neon
