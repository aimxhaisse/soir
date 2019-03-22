#ifndef SOIR_H
#define SOIR_H

#include <vector>

#include <SFML/Graphics.hpp>

#include "config.h"
#include "gfx.h"
#include "midi.h"
#include "status.h"

namespace soir {

class Context {
public:
  Config *core_config = nullptr;
  sf::RenderWindow *window = nullptr;
};

class Soir {
public:
  Status Init();
  Status Run();

private:
  Status InitWindow();
  Status InitMods();

  Context context_;
  std::unique_ptr<Config> core_config_;
  std::unique_ptr<Config> mods_config_;
  std::unique_ptr<sf::RenderWindow> window_;
  std::unique_ptr<MidiRouter> midi_router_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

} // namespace soir

#endif // SOIR_H
