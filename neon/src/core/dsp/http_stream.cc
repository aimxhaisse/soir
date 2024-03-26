#include <absl/log/log.h>

#include "http_stream.hh"
#include "vorbis_encoder.hh"

namespace neon {
namespace dsp {

HttpStream::HttpStream() : initialized_(false) {}

HttpStream::~HttpStream() {}

absl::Status HttpStream::PushAudioBuffer(const AudioBuffer& samples) {
  std::unique_lock<std::mutex> lock(mutex_);

  stream_.push_back(samples);
  cond_.notify_one();

  return absl::OkStatus();
}

absl::Status HttpStream::Encode(httplib::DataSink& sink) {
  Writer writer = [&sink](const void* data, std::size_t size) {
    return sink.write(reinterpret_cast<const char*>(data), size);
  };

  if (!initialized_) {
    initialized_ = true;
    auto status = encoder_.Init(writer);
    if (!status.ok()) {
      return status;
    }
  }

  std::list<AudioBuffer> stream;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock);
    std::swap(stream, stream_);
  }

  for (auto& buffer : stream) {
    auto status = encoder_.Encode(buffer, writer);
    if (!status.ok()) {
      return status;
    }
  }

  return absl::OkStatus();
}

}  // namespace dsp
}  // namespace neon
