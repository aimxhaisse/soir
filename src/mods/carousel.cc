#include <boost/filesystem.hpp>
#include <glog/logging.h>

#include "mods/carousel.h"
#include "soir.h"

using namespace std::placeholders;

namespace soir {

ModCarousel::ModCarousel(Context &ctx) : Mod(ctx), scaling_mode_(NONE) {}

Status ModCarousel::InitScalingMode(const Config &config) {
  const std::string mode = config.Get<std::string>("params.scaling");

  if (!mode.empty()) {
    if (mode == "none") {
      scaling_mode_ = NONE;
    } else if (mode == "auto") {
      scaling_mode_ = AUTO;
    } else {
      RETURN_ERROR(INVALID_CONFIG_FILE,
                   "Invalid scaling mode '" << mode << "'");
    }
  }

  return StatusCode::OK;
}

Status ModCarousel::LoadTextures(const Config &config) {
  boost::filesystem::path dir = config.Get<std::string>("params.directory");

  // For obscure reasons we have to use boost here because current OSX
  // version for C++17 doesn't have support for the filesystem module.
  try {
    for (auto it = boost::filesystem::directory_iterator{dir};
         it != boost::filesystem::directory_iterator{}; ++it) {
      const std::string full_path = it->path().string();
      const std::string name = it->path().leaf().string();

      std::unique_ptr<sf::Texture> texture = std::make_unique<sf::Texture>();
      if (!texture->loadFromFile(full_path)) {
        LOG(WARNING) << "Unable to load image from '" << full_path
                     << "', skipped.";
      } else {
        LOG(INFO) << "Loaded image '" << name << "'.";
        textures_[name] = std::move(texture);
      }
    }
  } catch (const boost::filesystem::filesystem_error &e) {
    RETURN_ERROR(INVALID_CONFIG_FILE,
                 "Unable to list files from directory " << dir << '.');
  }

  if (textures_.empty()) {
    RETURN_ERROR(INVALID_CONFIG_FILE,
                 "Unable to find images in directory " << dir << '.');
  }

  current_texture_ = textures_.begin();

  return StatusCode::OK;
}

Status ModCarousel::Init(const Config &config) {
  RegisterCallback("switchImage",
                   Callback(std::bind(&ModCarousel::SwitchImage, this, _1)));

  RETURN_IF_ERROR(BindCallbacks(config));
  RETURN_IF_ERROR(InitScalingMode(config));
  RETURN_IF_ERROR(LoadTextures(config));

  return StatusCode::OK;
}

void ModCarousel::SwitchImage(const MidiMessage &) {
  ++current_texture_;
  if (current_texture_ == textures_.end()) {
    current_texture_ = textures_.begin();
  }
}

void ModCarousel::Render() {
  sf::Texture texture = *current_texture_->second;

  auto sprite = sf::Sprite(texture);

  if (scaling_mode_ == AUTO) {
    const double ratio_x = static_cast<double>(ctx_.BufferWidth()) /
                           static_cast<double>(texture.getSize().x);
    const double ratio_y = static_cast<double>(ctx_.BufferHeight()) /
                           static_cast<double>(texture.getSize().y);

    sprite.setScale(ratio_x, ratio_y);
  }

  ctx_.CurrentTexture()->draw(sprite);
}

} // namespace soir
