#ifndef SOIR_H
#define SOIR_H

#include <SFML/Window.hpp>

#include "config.h"
#include "midi.h"
#include "status.h"

namespace soir {

class Soir {
public:
  // Initializes Soir:
  //
  // - loads configuration file,
  // - sets up the window.
  Status Init();

  // Main loop of Soir:
  //
  // - polls MIDI events,
  // - refreshes the screen.
  Status Run();

private:
  // Initializes the main window.
  Status InitWindow();

  std::unique_ptr<Config> core_config_;
  std::unique_ptr<MidiRouter> midi_router_;
  std::unique_ptr<sf::Window> window_;
};

} // namespace soir

#endif // SOIR_H
