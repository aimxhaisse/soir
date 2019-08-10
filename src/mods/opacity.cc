#include <glog/logging.h>

#include "mods/opacity.h"

namespace soir {

ModOpacity::ModOpacity(Context &ctx) : Mod(ctx), shader_(ctx), opacity_(0.0) {}

Status ModOpacity::Init(const Config &config) {
  opacity_ = config.Get<float>("params.value", 0);
  return shader_.Init(config);
}

void ModOpacity::Render() {
  shader_.GetShader().setUniform("opacity", opacity_);
  shader_.Render();
}

} // namespace soir
