#ifndef SOIR_MOD_NOISE_H
#define SOIR_MOD_NOISE_H

#include "gfx.h"

namespace soir {

class ModNoise : public Mod {
public:
  static constexpr const char *kModName = "noise";

  ModNoise(Context &ctx);
  Status Init(const Config &config);
  void Render();

private:
  void SwitchMode(const MidiMessage &);
  void SwitchMonochrome(const MidiMessage &);
  void SwitchColor(const MidiMessage &);

  enum Mode {
    MONOCHROME,
    COLOR,
  };

  int iter_;
  Mode mode_;
  std::vector<std::unique_ptr<sf::Uint8[]>> monochrome_;
  std::vector<std::unique_ptr<sf::Uint8[]>> color_;
};

} // namespace soir

#endif // SOIR_MOD_NOISE_H
