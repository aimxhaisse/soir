#ifndef SOIR_H
#define SOIR_H

#include <vector>

#include <SFML/Window.hpp>

#include "config.h"
#include "midi.h"
#include "mod.h"
#include "status.h"

namespace soir {

class Soir {
public:
  Status Init();
  Status Run();

private:
  Status InitWindow();
  Status InitMods();

  std::unique_ptr<Config> core_config_;
  std::unique_ptr<Config> mods_config_;
  std::vector<std::unique_ptr<Layer>> layers_;
  std::unique_ptr<MidiRouter> midi_router_;
  std::unique_ptr<sf::Window> window_;
};

} // namespace soir

#endif // SOIR_H
