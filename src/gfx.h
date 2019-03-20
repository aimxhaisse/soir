#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "config.h"
#include "status.h"

namespace soir {

class Mod {
public:
  virtual ~Mod() {}
  virtual Status Init(const Config &config) = 0;
  virtual void Render(sf::RenderWindow &window) = 0;
  static StatusOr<std::unique_ptr<Mod>> MakeMod(const std::string &type);
};

class Layer {
public:
  void AppendMod(std::unique_ptr<Mod> mod);
  void Render(sf::RenderWindow &window);

private:
  std::vector<std::unique_ptr<Mod>> mods_;
};

class ModText : public Mod {
public:
  Status Init(const Config &config);
  void Render(sf::RenderWindow &window);

private:
  sf::Font font_;
  sf::Text text_;
};

class ModDebug : public Mod {
public:
  Status Init(const Config &config);
  void Render(sf::RenderWindow &window);
};

} // namespace soir

#endif // SOIR_MOD_H
