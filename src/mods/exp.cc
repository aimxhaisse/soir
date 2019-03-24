#include "exp.h"
#include "soir.h"

namespace soir {

ModExp::ModExp(Context &ctx) : Mod(ctx) {}

Status ModExp::Init(const Config &config) {
  if (!sf::Shader::isAvailable()) {
    RETURN_ERROR(INTERNAL_GFX_ERROR,
                 "Shaders aren't supported on this hardware");
  }
  if (!shader_.loadFromFile("etc/shaders/exp.vert", sf::Shader::Fragment)) {
    RETURN_ERROR(INVALID_SHADER_FILE,
                 "Unable to load shader file etc/shaders/exp.vert");
  }

  overlay_.setSize(sf::Vector2f(ctx_.BufferWidth(), ctx_.BufferHeight()));
  overlay_.setFillColor(sf::Color::Red);

  return StatusCode::OK;
}

void ModExp::Render() { ctx_.CurrentTexture()->draw(overlay_, &shader_); }

} // namespace soir
