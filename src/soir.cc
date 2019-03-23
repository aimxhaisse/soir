#include <glog/logging.h>

#include "soir.h"

namespace soir {

constexpr const char *kCoreConfigPath = "etc/soir.yml";
constexpr const char *kModsConfigPath = "etc/mods.yml";

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

// Default for resize mode -- core.resize
constexpr bool kDefaultResize = false;

Config *Context::CoreConfig() { return core_config_; }

sf::RenderWindow *Context::Window() { return window_; }

sf::Texture *Context::CurrentTexture() { return current_txt_; }

sf::Sprite *Context::CurrentSprite() { return current_sprite_; }

MidiRouter *Context::Router() { return midi_router_; }

int Context::WindowWidth() const { return window_->getSize().x; }

int Context::WindowHeight() const { return window_->getSize().y; }

int Context::Width() const { return width_; }

int Context::Height() const { return height_; }

Status Soir::Init() {
  MOVE_OR_RETURN(core_config_, Config::LoadFromPath(kCoreConfigPath));
  MOVE_OR_RETURN(mods_config_, Config::LoadFromPath(kModsConfigPath));

  midi_router_ = std::make_unique<MidiRouter>();

  RETURN_IF_ERROR(midi_router_->Init());
  RETURN_IF_ERROR(InitWindow());

  ctx_.core_config_ = core_config_.get();
  ctx_.window_ = window_.get();
  ctx_.midi_router_ = midi_router_.get();

  RETURN_IF_ERROR(InitMods());

  return StatusCode::OK;
}

Status Soir::InitWindow() {
  const int width = core_config_->Get<int>("core.width", kDefaultWidth);
  const int height = core_config_->Get<int>("core.height", kDefaultHeight);
  const std::string title =
      core_config_->Get<std::string>("core.title", kDefaultTitle);

  const bool is_fullscreen =
      core_config_->Get<bool>("core.fullscreen", kDefaultFullscreen);
  unsigned int style = sf::Style::Titlebar;
  if (is_fullscreen) {
    style |= sf::Style::Fullscreen;
  }

  const bool is_resize = core_config_->Get<bool>("core.resize", kDefaultResize);
  if (is_resize) {
    style |= sf::Style::Resize;
  }

  window_ = std::make_unique<sf::RenderWindow>(sf::VideoMode(width, height),
                                               title, style);

  window_->setVerticalSyncEnabled(true);
  window_->setFramerateLimit(
      core_config_->Get<int>("core.fps_max", kDefaultFpsMax));

  return StatusCode::OK;
}

Status Soir::InitMods() {
  for (const auto &layers_config : mods_config_->GetConfigs("root")) {
    std::unique_ptr<Layer> layer = std::make_unique<Layer>(ctx_);
    RETURN_IF_ERROR(layer->Init());
    for (const auto &mod_config : layers_config->GetConfigs("units")) {
      const std::string mod_type = mod_config->Get<std::string>("type");
      std::unique_ptr<Mod> mod;
      MOVE_OR_RETURN(mod, Mod::MakeMod(ctx_, mod_type));
      RETURN_IF_ERROR(mod->Init(*mod_config));
      layer->AppendMod(std::move(mod));
    }
    layers_.emplace_back(std::move(layer));
  }
  return StatusCode::OK;
}

Status Soir::Run() {
  while (window_->isOpen()) {
    midi_router_->ProcessEvents();

    sf::Event event;
    while (window_->pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          event.key.code == sf::Keyboard::Escape) {
        LOG(INFO) << "Received exit event, leaving now.";
        window_->close();
      }
    }

    window_->clear(sf::Color::Black);
    for (auto &layer : layers_) {
      layer->Render();
    }
    window_->display();
  }

  return StatusCode::OK;
}

} // namespace soir
