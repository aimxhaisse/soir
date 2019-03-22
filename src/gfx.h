#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "config.h"
#include "status.h"

namespace soir {

class Context;

class Mod {
public:
  virtual ~Mod() {}
  virtual Status Init(Context &ctx, const Config &config) = 0;
  virtual void Render(Context &ctx) = 0;
  static StatusOr<std::unique_ptr<Mod>> MakeMod(const std::string &type);
};

class Layer {
public:
  void AppendMod(std::unique_ptr<Mod> mod);
  void Render(Context &ctx);

private:
  std::vector<std::unique_ptr<Mod>> mods_;
};

} // namespace soir

#endif // SOIR_MOD_H
