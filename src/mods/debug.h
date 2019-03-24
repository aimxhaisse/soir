#ifndef SOIR_MOD_DEBUG_H
#define SOIR_MOD_DEBUG_H

#include "gfx.h"

namespace soir {

class ModDebug : public Mod {
public:
  static constexpr const char *kModName = "debug";

  ModDebug(Context &ctx);
  Status Init(const Config &config);
  void Render();
};

} // namespace soir

#endif // SOIR_MOD_DEBUG_H
