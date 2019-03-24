#ifndef SOIR_MOD_TEXT_H
#define SOIR_MOD_TEXT_H

#include "gfx.h"

namespace soir {

class ModText : public Mod {
public:
  static constexpr const char *kModName = "text";

  ModText(Context &ctx);
  Status Init(const Config &config) override;
  void Render() override;

private:
  void ShiftLetters(const MidiMessage &);

  sf::Font font_;
  sf::Text text_;
};

} // namespace soir

#endif // SOIR_MOD_TEXT_H
