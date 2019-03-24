#include <cstdlib>

#include <glog/logging.h>

#include "mods/noise.h"
#include "soir.h"

using namespace std::placeholders;

namespace soir {

namespace {
constexpr int kFrameNumber = 10;
}

ModNoise::ModNoise(Context &ctx) : Mod(ctx), iter_(0), mode_(MONOCHROME) {}

Status ModNoise::Init(const Config &config) {
  RegisterCallback("switchMode",
                   Callback(std::bind(&ModNoise::SwitchMode, this, _1)));
  RegisterCallback("switchMonochrome",
                   Callback(std::bind(&ModNoise::SwitchMonochrome, this, _1)));
  RegisterCallback("switchColor",
                   Callback(std::bind(&ModNoise::SwitchColor, this, _1)));
  RETURN_IF_ERROR(BindCallbacks(config));

  monochrome_.reserve(kFrameNumber);
  color_.reserve(kFrameNumber);

  const int w = ctx_.BufferWidth();
  const int h = ctx_.BufferHeight();

  for (int i = 0; i < kFrameNumber; ++i) {
    const int buffer_size = w * h * 4;
    auto mono = std::make_unique<sf::Uint8[]>(buffer_size);
    auto color = std::make_unique<sf::Uint8[]>(buffer_size);
    for (int x = 0; x < w; ++x) {
      for (int y = 0; y < h; ++y) {
        const auto r = static_cast<sf::Uint8>(std::rand());
        const auto g = static_cast<sf::Uint8>(std::rand());
        const auto b = static_cast<sf::Uint8>(std::rand());
        const auto a = static_cast<sf::Uint8>(std::rand());
        const int pos = (y * w * 4) + (x * 4);

        mono.get()[pos] = r;
        mono.get()[pos + 1] = r;
        mono.get()[pos + 2] = r;
        mono.get()[pos + 3] = r;

        color.get()[pos] = r;
        color.get()[pos + 1] = g;
        color.get()[pos + 2] = b;
        color.get()[pos + 3] = a;
      }
    }

    auto mono_txt = std::make_unique<sf::Texture>();
    if (!mono_txt->create(w, h)) {
      RETURN_ERROR(INTERNAL_GFX_ERROR,
                   "Unable to create texture of size " << w << "x" << h);
    }
    mono_txt->update(mono.get(), w, h, 0, 0);
    monochrome_.emplace_back(std::move(mono_txt));

    auto color_txt = std::make_unique<sf::Texture>();
    if (!color_txt->create(w, h)) {
      RETURN_ERROR(INTERNAL_GFX_ERROR,
                   "Unable to create texture of size " << w << "x" << h);
    }
    color_txt->update(color.get(), w, h, 0, 0);
    color_.emplace_back(std::move(color_txt));
  }
  return StatusCode::OK;
}

void ModNoise::SwitchMode(const MidiMessage &m) {
  if (mode_ == MONOCHROME) {
    mode_ = COLOR;
  } else {
    mode_ = MONOCHROME;
  }
}

void ModNoise::SwitchMonochrome(const MidiMessage &) { mode_ = MONOCHROME; }

void ModNoise::SwitchColor(const MidiMessage &) { mode_ = COLOR; }

void ModNoise::Render() {
  iter_ = (iter_ + 1) % kFrameNumber;

  sf::Texture &txt = (mode_ == MONOCHROME) ? *(monochrome_[iter_].get())
                                           : *(color_[iter_].get());

  ctx_.CurrentTexture()->draw(sf::Sprite(txt));
}

} // namespace soir
