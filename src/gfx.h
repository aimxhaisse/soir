#ifndef SOIR_MOD_H
#define SOIR_MOD_H

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "config.h"
#include "status.h"

namespace soir {

using MidiMessage = std::vector<unsigned char>;

class Context;

// Custom class for callbacks as we need to be able to compare it to
// unregister safely modules.
class Callback {
public:
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
  Mod(Context &ctx) : ctx_(ctx) {}
  virtual ~Mod();

  // This can be called in the Init() to register a callback that will
  // be triggered upon the given MIDI mnemo. Once the Init() is done,
  // the callback may start firing.
  Status BindCallback(const std::string &mnemo, const Callback &cb);

  virtual Status Init(const Config &config) { return StatusCode::OK; }
  virtual void Render() {}

  static StatusOr<std::unique_ptr<Mod>> MakeMod(Context &ctx,
                                                const std::string &type);

protected:
  Context &ctx_;

private:
  std::vector<Callback> callbacks_;
};

class Layer {
public:
  void AppendMod(std::unique_ptr<Mod> mod);
  void Render();

private:
  std::vector<std::unique_ptr<Mod>> mods_;
};

} // namespace soir

#endif // SOIR_MOD_H
