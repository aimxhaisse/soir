#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <string>
#include <vector>

#include "config.h"
#include "status.h"

namespace soir {

class Mod {
public:
  virtual ~Mod() {}
  virtual Status Init(const Config &config) = 0;

  static StatusOr<std::unique_ptr<Mod>> MakeMod(const std::string &type);
};

class Layer {
public:
  void AppendMod(std::unique_ptr<Mod> mod);

private:
  std::vector<std::unique_ptr<Mod>> mods_;
};

class ModText : public Mod {
public:
  Status Init(const Config &config);
};

class ModDebug : public Mod {
public:
  Status Init(const Config &config);
};

} // namespace soir

#endif // SOIR_MOD_H
