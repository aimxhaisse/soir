#include <absl/log/log.h>

#include "http.hh"

namespace maethstro {
namespace soir {

HttpStream::HttpStream() {}

HttpStream::~HttpStream() {}

absl::Status HttpStream::PushAudioBuffer(const AudioBuffer& samples) {
  std::unique_lock<std::mutex> lock(mutex_);
  stream_.push_back(samples);
  cond_.notify_one();
  return absl::OkStatus();
}

absl::Status HttpStream::Run(httplib::Response& response) {
  LOG(INFO) << "Starting HTTP stream";

  while (true) {
    std::list<AudioBuffer> stream;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cond_.wait(lock);
      if (!running_) {
        break;
      }
      std::swap(stream, stream_);
    }

    // Here we need to encode stream and send it to response.

    stream.clear();
  }

  return absl::OkStatus();
}

absl::Status HttpStream::Stop() {
  LOG(INFO) << "Stopping HTTP stream";

  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  cond_.notify_all();

  LOG(INFO) << "HTTP stream stopped";

  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
