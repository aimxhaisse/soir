#ifndef SOIR_MOD_EXP_H
#define SOIR_MOD_EXP_H

#include "gfx.h"

namespace soir {

class ModExp : public Mod {
public:
  static constexpr const char *kModName = "exp";

  ModExp(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  sf::RectangleShape overlay_;
  sf::Shader shader_;
};

} // namespace soir

#endif // SOIR_MOD_EXP_H
