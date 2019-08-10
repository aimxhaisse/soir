#ifndef SOIR_MOD_CAROUSEL_H
#define SOIR_MOD_CAROUSEL_H

#include <map>

#include "gfx.h"

namespace soir {

class ModCarousel : public Mod {
public:
  static constexpr const char *kModName = "carousel";

  ModCarousel(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  Status InitScalingMode(const Config &config);
  Status LoadTextures(const Config &config);

  void SwitchImage(const MidiMessage &);

  enum ScalingMode {
    NONE,
    AUTO,
  };

  // Map of image names to textures.
  using Textures = std::map<std::string, std::unique_ptr<sf::Texture>>;

  Textures textures_;
  Textures::iterator current_texture_;
  ScalingMode scaling_mode_;
};

} // namespace soir

#endif // SOIR_MOD_CAROUSEL_H
