#include "audio/audio_recorder.hh"

#include <absl/log/log.h>

#include <filesystem>

namespace soir {

AudioRecorder::AudioRecorder()
    : is_recording_(false),
      stop_(false),
      write_failed_(false),
      queue_(16),
      total_frames_(0) {}

AudioRecorder::~AudioRecorder() { MaybeStop().IgnoreError(); }

absl::Status AudioRecorder::Init(const std::string& file_path) {
  if (file_path == file_path_ && is_recording_.load()) {
    return absl::OkStatus();
  }

  auto status = MaybeStop();
  if (!status.ok()) {
    LOG(WARNING) << "Failed to stop previous recording: " << status;
  }

  std::filesystem::path file_path_obj(file_path);
  std::filesystem::path dir_path = file_path_obj.parent_path();
  if (!dir_path.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(dir_path, ec);
    if (ec) {
      return absl::InternalError("Failed to create directory: " + ec.message());
    }
  }

  file_.open(file_path, std::ios::binary | std::ios::trunc);
  if (!file_.is_open()) {
    return absl::InternalError("Failed to open " + file_path + " for writing");
  }

  // 32-bit float WAV header with zero sizes, patched on stop.
  WriteWavHeader(file_, 0);

  total_frames_ = 0;
  write_failed_ = false;
  stop_ = false;
  file_path_ = file_path;
  is_recording_ = true;

  writer_thread_ = std::thread([this]() { WriterLoop(); });

  LOG(INFO) << "Started audio recording to: " << file_path;
  return absl::OkStatus();
}

absl::Status AudioRecorder::MaybeStop() {
  if (!is_recording_.exchange(false)) {
    return absl::OkStatus();
  }

  stop_ = true;
  if (writer_thread_.joinable()) {
    writer_thread_.join();
  }

  if (write_failed_.load()) {
    LOG(WARNING) << "Audio recording aborted: disk write failed (" << file_path_
                 << ")";
  }

  // Patch the RIFF/data sizes with the real frame count.
  file_.seekp(0);
  WriteWavHeader(file_, total_frames_);
  file_.flush();
  const bool ok = file_.good();
  file_.close();
  if (!ok) {
    return absl::InternalError("Failed to finalize " + file_path_);
  }

  LOG(INFO) << "Stopped audio recording to: " << file_path_ << " ("
            << total_frames_ << " frames)";
  return absl::OkStatus();
}

absl::Status AudioRecorder::PushAudioBuffer(AudioBuffer& buffer) {
  if (!is_recording_.load()) {
    return absl::OkStatus();
  }

  const float* left = buffer.GetChannel(kLeftChannel);
  const float* right = buffer.GetChannel(kRightChannel);
  for (size_t i = 0; i < buffer.Size(); i++) {
    queue_.enqueue(left[i]);
    queue_.enqueue(right[i]);
  }

  return absl::OkStatus();
}

void AudioRecorder::WriterLoop() {
  std::vector<float> chunk;
  chunk.reserve(8192);
  float sample;

  auto flush = [this, &chunk]() {
    if (chunk.empty() || write_failed_.load()) {
      chunk.clear();
      return;
    }
    if (!WriteChunk(chunk)) {
      write_failed_ = true;
    }
    chunk.clear();
  };

  while (true) {
    if (!queue_.wait_dequeue_timed(sample, 1000)) {
      if (stop_.load()) {
        break;
      }
      continue;
    }

    if (!write_failed_.load()) {
      chunk.push_back(sample);
      if (chunk.size() >= 8192) {
        flush();
      }
    }
  }

  // Drain the queue so Init can restart from an empty state.
  while (queue_.try_dequeue(sample)) {
    if (write_failed_.load()) {
      continue;
    }
    chunk.push_back(sample);
    if (chunk.size() >= 8192) {
      flush();
    }
  }
  flush();
}

bool AudioRecorder::WriteChunk(const std::vector<float>& chunk) {
  file_.write(reinterpret_cast<const char*>(chunk.data()),
              static_cast<std::streamsize>(chunk.size() * sizeof(float)));
  if (!file_) {
    LOG(ERROR) << "Failed to write to " << file_path_ << ": disk error";
    return false;
  }
  // Interleaved samples are always written in stereo pairs.
  total_frames_ += chunk.size() / kNumChannels;
  return true;
}

void AudioRecorder::WriteWavHeader(std::ofstream& out, uint64_t frames) {
  const uint32_t channels = kNumChannels;
  const uint32_t sample_rate = static_cast<uint32_t>(kSampleRate);
  const uint32_t bytes_per_frame = channels * 4;
  const uint32_t data_size = static_cast<uint32_t>(frames * bytes_per_frame);

  auto put32 = [](std::ofstream& o, uint32_t v) {
    const char b[4] = {static_cast<char>(v & 0xff),
                       static_cast<char>((v >> 8) & 0xff),
                       static_cast<char>((v >> 16) & 0xff),
                       static_cast<char>((v >> 24) & 0xff)};
    o.write(b, 4);
  };
  auto put16 = [](std::ofstream& o, uint16_t v) {
    const char b[2] = {static_cast<char>(v & 0xff),
                       static_cast<char>((v >> 8) & 0xff)};
    o.write(b, 2);
  };

  out.write("RIFF", 4);
  put32(out, 36 + data_size);
  out.write("WAVEfmt ", 8);
  put32(out, 16);
  put16(out, 3);  // IEEE float
  put16(out, channels);
  put32(out, sample_rate);
  put32(out, sample_rate * bytes_per_frame);
  put16(out, bytes_per_frame);
  put16(out, 32);
  out.write("data", 4);
  put32(out, data_size);
}

}  // namespace soir
