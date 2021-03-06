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
  friend class Layer;

public:
  Config *CoreConfig();
  sf::RenderWindow *Window();
  MidiRouter *Router();

  sf::RenderTexture *CurrentTexture();
  Layer *CurrentLayer();
  const sf::Clock &Clock() const;

  // Size of the window.
  int WindowWidth() const;
  int WindowHeight() const;

  // Size of the texture, this can be different in case the window is
  // resized or if we want to get cool stretching effects.
  int BufferWidth() const;
  int BufferHeight() const;

private:
  // This is somewhat a lie, this is not updated whenever the window
  // is resized because SFML already handles scales automatically. We
  // can consider the window size as being the one set from the
  // beginning, so this is cached.
  int window_width_ = 0;
  int window_height_ = 0;

  int buffer_width_ = 0;
  int buffer_height_ = 0;

  Config *core_config_ = nullptr;
  sf::RenderWindow *window_ = nullptr;
  Layer *current_layer_ = nullptr;
  MidiRouter *midi_router_ = nullptr;

  sf::Clock clock_;
};

class Soir {
public:
  Status Init();
  Status Run();

private:
  Status InitWindow();
  Status InitMods();

  std::unique_ptr<Config> core_config_;
  std::unique_ptr<sf::RenderWindow> window_;
  std::unique_ptr<MidiRouter> midi_router_;

  // Order is important here, layers need to be destroyed prior MIDI
  // router and context.
  Context ctx_;
  std::unique_ptr<Config> mods_config_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

} // namespace soir

#endif // SOIR_H
