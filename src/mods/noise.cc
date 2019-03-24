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

  const int w = ctx_.Width();
  const int h = ctx_.Height();

  for (int i = 0; i < kFrameNumber; ++i) {
    const int buffer_size = w * h * 4;
    monochrome_.emplace_back(std::make_unique<sf::Uint8[]>(buffer_size));
    color_.emplace_back(std::make_unique<sf::Uint8[]>(buffer_size));
    for (int x = 0; x < w; ++x) {
      for (int y = 0; y < h; ++y) {
        const auto r = static_cast<sf::Uint8>(std::rand());
        const auto g = static_cast<sf::Uint8>(std::rand());
        const auto b = static_cast<sf::Uint8>(std::rand());
        const auto a = static_cast<sf::Uint8>(std::rand());
        const int pos = (y * w * 4) + (x * 4);

        monochrome_[i].get()[pos] = r;
        monochrome_[i].get()[pos + 1] = r;
        monochrome_[i].get()[pos + 2] = r;
        monochrome_[i].get()[pos + 3] = r;

        color_[i].get()[pos] = r;
        color_[i].get()[pos + 1] = g;
        color_[i].get()[pos + 2] = b;
        color_[i].get()[pos + 3] = a;
      }
    }
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
  const int w = ctx_.Width();
  const int h = ctx_.Height();
  auto &buffers = mode_ == MONOCHROME ? monochrome_ : color_;
  ctx_.CurrentTexture()->update(buffers[iter_ % kFrameNumber].get(), w, h, 0,
                                0);
  ++iter_;
}

} // namespace soir
