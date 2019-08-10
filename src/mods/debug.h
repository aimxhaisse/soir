#ifndef SOIR_MOD_DEBUG_H
#define SOIR_MOD_DEBUG_H

#include <SFML/System/Clock.hpp>

#include "gfx.h"

namespace soir {

class ModDebug : public Mod {
public:
  static constexpr const char *kModName = "debug";

  ModDebug(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  Status InitFps(const Config &config);
  void RenderFps();

  sf::Font font_;

  sf::Clock fps_clock_;
  sf::Text fps_text_;
  int current_fps_;
  int elapsed_fps_;
};

} // namespace soir

#endif // SOIR_MOD_DEBUG_H
