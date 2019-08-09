#include <boost/filesystem.hpp>
#include <glog/logging.h>

#include "mods/carousel.h"
#include "soir.h"

using namespace std::placeholders;

namespace soir {

ModCarousel::ModCarousel(Context &ctx) : Mod(ctx) {}

Status ModCarousel::Init(const Config &config) {
  RegisterCallback("switchImage",
                   Callback(std::bind(&ModCarousel::SwitchImage, this, _1)));
  RETURN_IF_ERROR(BindCallbacks(config));

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

void ModCarousel::SwitchImage(const MidiMessage &) {
  ++current_texture_;
  if (current_texture_ == textures_.end()) {
    current_texture_ = textures_.begin();
  }
}

void ModCarousel::Render() {
  ctx_.CurrentTexture()->draw(sf::Sprite(*current_texture_->second));
}

} // namespace soir
