#ifndef SOIR_H
#define SOIR_H

#include <vector>

#include <SFML/Graphics.hpp>

#include "config.h"
#include "gfx.h"
#include "midi.h"
#include "status.h"

namespace soir {

class Soir;

// Global-like object passed to all modules to draw on the window.
class Context {
  friend class Soir;

public:
  Config *CoreConfig();
  sf::RenderWindow *Window();
  MidiRouter *Router();

private:
  Config *core_config_ = nullptr;
  sf::RenderWindow *window_ = nullptr;
  MidiRouter *midi_router_;
};

class Soir {
public:
  Status Init();
  Status Run();

private:
  Status InitWindow();
  Status InitMods();

  Context ctx_;
  std::unique_ptr<Config> core_config_;
  std::unique_ptr<Config> mods_config_;
  std::vector<std::unique_ptr<Layer>> layers_;
  std::unique_ptr<sf::RenderWindow> window_;
  std::unique_ptr<MidiRouter> midi_router_;
};

} // namespace soir

#endif // SOIR_H
