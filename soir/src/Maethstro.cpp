#include "Maethstro.hh"

#include <glog/logging.h>

#include <chrono>
#include <csignal>
#include <thread>

namespace maethstro {

std::atomic<bool> stop(false);

namespace {
void SignalHandler(int signal) {
  LOG(INFO) << "received signal=" << signal << ", stopping...";
  stop = true;
}
}  // namespace

Status Maethstro::Init(const std::string& path) {
  MOVE_OR_RETURN(configParser_, ConfigParser::LoadFromPath(path));
  MOVE_OR_RETURN(config_, Config::LoadFromParser(*configParser_));
  MOVE_OR_RETURN(liquidifier_, LiquidPusher::InitFromConfig(*config_));
  MOVE_OR_RETURN(renderer_,
                 Renderer::InitFromConfig(*config_, liquidifier_.get()));
  MOVE_OR_RETURN(service_,
                 NetworkService::InitFromConfig(*config_, renderer_.get()));

  LOG(INFO) << "installing signal handler";

  std::signal(SIGTERM, SignalHandler);
  std::signal(SIGINT, SignalHandler);

  return StatusCode::OK;
}

Status Maethstro::Run() {
  std::thread midi([this]() {
    service_->Loop(stop);
    stop = true;
  });

  std::thread renderer([this]() {
    renderer_->Loop(stop);
    stop = true;
  });

  std::thread pusher([this]() {
    liquidifier_->Loop(stop);
    stop = true;
  });

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (stop) {
      LOG(INFO) << "waiting for midi thread to stop";
      service_->Stop();
      midi.join();

      LOG(INFO) << "waiting for renderer thread to stop";
      renderer.join();

      LOG(INFO) << "waiting for liquidsoap thread to stop";
      pusher.join();

      return StatusCode::OK;
    }
  }

  return StatusCode::OK;
}

}  // namespace maethstro
