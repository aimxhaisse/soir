#include "shader.h"
#include "soir.h"

namespace soir {

ModShader::ModShader(Context &ctx) : Mod(ctx) {}

Status ModShader::Init(const Config &config) {
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

  texture_ = std::make_unique<sf::RenderTexture>();
  if (!texture_->create(ctx_.BufferWidth(), ctx_.BufferHeight())) {
    RETURN_ERROR(INTERNAL_GFX_ERROR,
                 "Unable to create shader texture with width="
                     << ctx_.BufferWidth()
                     << ", height=" << ctx_.BufferHeight());
  }

  return StatusCode::OK;
}

void ModShader::Render() {
  shader_.setUniform("texture", sf::Shader::CurrentTexture);
  texture_->clear();
  texture_->draw(sf::Sprite(ctx_.CurrentTexture()->getTexture()), &shader_);
  ctx_.CurrentLayer()->SwapTexture(texture_);
}

} // namespace soir
