#ifndef SOIR_MOD_TEXT_H
#define SOIR_MOD_TEXT_H

#include "gfx.h"

namespace soir {

class ModText : public Mod {
public:
  Status Init(const Config &config);
  void Render(sf::RenderWindow &window);

private:
  sf::Font font_;
  sf::Text text_;
};

} // namespace soir

#endif // SOIR_MOD_TEXT_H
