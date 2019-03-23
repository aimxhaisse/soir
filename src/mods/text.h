#ifndef SOIR_MOD_TEXT_H
#define SOIR_MOD_TEXT_H

#include "gfx.h"

namespace soir {

class ModText : public Mod {
public:
  ModText(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  void OnEvent(const MidiMessage &message);

  sf::Font font_;
  sf::Text text_;
};

} // namespace soir

#endif // SOIR_MOD_TEXT_H
