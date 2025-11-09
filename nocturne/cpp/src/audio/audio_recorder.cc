#include "audio/audio_recorder.hh"

#include <absl/log/log.h>

#include <filesystem>

namespace soir {

AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder() { MaybeStop().IgnoreError(); }

absl::Status AudioRecorder::Init(const std::string& file_path) {
  // Stop any existing recording first
  auto status = MaybeStop();
  if (!status.ok()) {
    LOG(WARNING) << "Failed to stop previous recording: " << status;
  }

  std::lock_guard<std::mutex> lock(buffer_mutex_);

  // Already initialized with the same file path
  if (file_path == file_path_) {
    return absl::OkStatus();
  }

  file_path_ = file_path;

  // Create directory if it doesn't exist
  std::filesystem::path file_path_obj(file_path_);
  std::filesystem::path dir_path = file_path_obj.parent_path();
  if (!dir_path.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(dir_path, ec);
    if (ec) {
      return absl::InternalError("Failed to create directory: " + ec.message());
    }
  }

  // Set up audio file properties for streaming
  audio_file_.setAudioBufferSize(kNumChannels, 0);
  audio_file_.setSampleRate(kSampleRate);
  audio_file_.setBitDepth(32);  // 32-bit float

  // Initialize empty samples vectors for streaming
  audio_file_.samples.resize(kNumChannels);

  is_recording_ = true;

  LOG(INFO) << "Started audio recording to: " << file_path_;
  return absl::OkStatus();
}

absl::Status AudioRecorder::MaybeStop() {
  std::lock_guard<std::mutex> lock(buffer_mutex_);

  if (!is_recording_) {
    return absl::OkStatus();  // Not recording, nothing to do
  }

  is_recording_ = false;

  // Save the accumulated audio data to file
  if (!audio_file_.save(file_path_)) {
    return absl::InternalError("Failed to save audio file: " + file_path_);
  }

  LOG(INFO) << "Saved audio recording to: " << file_path_ << " ("
            << audio_file_.getNumSamplesPerChannel() << " samples)";

  // Clear the audio file data
  audio_file_.samples.clear();

  return absl::OkStatus();
}

absl::Status AudioRecorder::PushAudioBuffer(AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(buffer_mutex_);

  if (!is_recording_) {
    return absl::OkStatus();  // Not recording, ignore
  }

  // Append buffer data directly to audio file samples
  for (int channel = 0; channel < kNumChannels; ++channel) {
    float* channel_data = buffer.GetChannel(channel);
    audio_file_.samples[channel].insert(audio_file_.samples[channel].end(),
                                        channel_data,
                                        channel_data + buffer.Size());
  }

  return absl::OkStatus();
}

}  // namespace soir
