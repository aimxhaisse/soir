#ifndef SOIR_MOD_DEBUG_H
#define SOIR_MOD_DEBUG_H

#include "gfx.h"

namespace soir {

class ModDebug : public Mod {
public:
  Status Init(const Config &config);
  void Render(sf::RenderWindow &window);
};

} // namespace soir

#endif // SOIR_MOD_DEBUG_H
