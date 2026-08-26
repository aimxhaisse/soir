#pragma once

#include <memory>
#include <string>
#include <thread>

#include "absl/status/status.h"

namespace httplib {
class Server;
}  // namespace httplib

namespace soir {
namespace audio {

class AudioStream;

class AudioHttpServer {
 public:
  AudioHttpServer();
  ~AudioHttpServer();

  absl::Status Start(AudioStream* stream, const std::string& host, int port);
  absl::Status Stop();

 private:
  std::unique_ptr<httplib::Server> server_;
  std::thread server_thread_;
  bool running_ = false;
};

}  // namespace audio
}  // namespace soir
