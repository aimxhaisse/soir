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
  void SwitchImage(const MidiMessage &);

  // Map of image names to textures.
  using Textures = std::map<std::string, std::unique_ptr<sf::Texture>>;

  Textures textures_;
  Textures::iterator current_texture_;
};

} // namespace soir

#endif // SOIR_MOD_CAROUSEL_H
