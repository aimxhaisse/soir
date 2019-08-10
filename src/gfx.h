#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <functional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "common.h"
#include "config.h"
#include "status.h"

namespace soir {

class Context;

// Custom class for callbacks as we need to be able to compare it to
// unregister safely modules.
class Callback {
public:
  Callback() {}
  explicit Callback(std::function<void(const MidiMessage &)> func);

  void SetId(const std::string &id);
  bool operator==(const Callback &rhs) const;
  void Call(const MidiMessage &msg);

private:
  std::string id_;
  std::function<void(const MidiMessage &)> func_;
};

class Mod {
public:
  explicit Mod(Context &ctx);
  virtual ~Mod();

  virtual Status Init(const Config &config) { return StatusCode::OK; }
  virtual void Render() = 0;

  static StatusOr<std::unique_ptr<Mod>> MakeMod(Context &ctx,
                                                const std::string &type);

protected:
  void RegisterCallback(const std::string &callback_name, const Callback &cb);
  Status BindCallbacks(const Config &mod_config);

  std::map<std::string, Callback> callbacks_;
  Context &ctx_;
};

class Layer {
public:
  explicit Layer(Context &ctx);

  Status Init();
  sf::RenderTexture *Texture();
  void SwapTexture(std::unique_ptr<sf::RenderTexture> &texture);
  void AppendMod(std::unique_ptr<Mod> mod);
  void Render();

private:
  sf::Sprite MakeSprite();

  Context &ctx_;
  std::vector<std::unique_ptr<Mod>> mods_;
  std::unique_ptr<sf::RenderTexture> texture_;
};

} // namespace soir

#endif // SOIR_MOD_H
