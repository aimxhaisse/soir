#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <string>
#include <vector>

#include "config.h"
#include "status.h"

namespace soir {

// Abstract class to represent a mod.
class Mod {
public:
  virtual ~Mod() {}

  // Factory method to create a mod from a specific type.
  static StatusOr<std::unique_ptr<Mod>> MakeMod(const std::string &type);

  virtual Status Init(const Config &config) = 0;
};

// List of mods sequentially called to modify a graphic layer.
class Layer {
public:
  void AppendMod(std::unique_ptr<Mod> mod);

private:
  std::vector<std::unique_ptr<Mod>> mods_;
};

// Sandbox for now, to be moved to a dedicated mod directory.
class ModText : public Mod {
public:
  Status Init(const Config &config);
};

} // namespace soir

#endif // SOIR_MOD_H