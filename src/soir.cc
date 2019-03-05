#include <glog/logging.h>

#include "soir.h"

namespace soir {

// Relative path to the core configuration file.
constexpr const char *kCoreConfigPath = "etc/soir.yml";

// Default width for the window -- core.width
constexpr const int kDefaultWidth = 400;

// Default height for the window -- core.height
constexpr const int kDefaultHeight = 300;

// Default value for maximum number of FPS -- core.max_fps
constexpr const int kDefaultFpsMax = 60;

// Default title for the window -- core.title
constexpr const char *kDefaultTitle = "Soir ~";

// Default for fullscreen mode -- core.fullscreen
constexpr bool kDefaultFullscreen = false;

Status Soir::Init() {
  MOVE_OR_RETURN(core_config_, Config::LoadFromPath(kCoreConfigPath));
  RETURN_IF_ERROR(InitWindow());

  return StatusCode::OK;
}

Status Soir::InitWindow() {
  const int width = core_config_->Get<int>("core.width", kDefaultWidth);
  const int height = core_config_->Get<int>("core.height", kDefaultHeight);
  const std::string title =
      core_config_->Get<std::string>("core.title", kDefaultTitle);

  const bool is_fullscreen =
      core_config_->Get<bool>("core.fullscreen", kDefaultFullscreen);
  unsigned int style = 0;
  if (is_fullscreen) {
    style |= sf::Style::Fullscreen;
  }

  window_ =
      std::make_unique<sf::Window>(sf::VideoMode(width, height), title, style);

  window_->setVerticalSyncEnabled(true);
  window_->setFramerateLimit(
      core_config_->Get<int>("core.fps_max", kDefaultFpsMax));

  return StatusCode::OK;
} // namespace soir

Status Soir::Run() {
  while (window_->isOpen()) {
    sf::Event event;
    while (window_->pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window_->close();
      }
    }
  }

  return StatusCode::OK;
}

} // namespace soir

using namespace soir;

int main(int ac, char **av) {
  FLAGS_logtostderr = 1;
  ::google::InitGoogleLogging(av[0]);

  Soir soir;
  Status status = soir.Init();
  if (status == StatusCode::OK) {
    status = soir.Run();
    LOG(INFO) << "Soir exited: " << status;
  } else {
    LOG(WARNING) << "Unable to init Soir: " << status;
  }

  return 0;
}
