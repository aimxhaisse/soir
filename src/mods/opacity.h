#ifndef SOIR_MOD_OPACITY_H
#define SOIR_MOD_OPACITY_H

#include "mods/shader.h"

#include "gfx.h"

namespace soir {

class ModOpacity : public Mod {
public:
  static constexpr const char *kModName = "opacity";

  ModOpacity(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  ModShader shader_;
  float opacity_;
};

} // namespace soir

#endif // SOIR_MOD_EXP_H
