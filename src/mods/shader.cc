#include <glog/logging.h>

#include "shader.h"
#include "soir.h"

namespace soir {

constexpr const char *kShaderPath = "etc/shaders/exp.vert";

ModShader::ModShader(Context &ctx) : Mod(ctx) {}

Status ModShader::Init(const Config &config) {
  if (!sf::Shader::isAvailable()) {
    RETURN_ERROR(INTERNAL_GFX_ERROR,
                 "Shaders aren't supported on this hardware");
  }

  MaybeReloadShader();

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

void ModShader::MaybeReloadShader() {
  auto new_version =
      boost::filesystem::last_write_time(boost::filesystem::path(kShaderPath));
  if (new_version != version_) {
    // Even if we fail to load it, we want to override the version
    // here so that we don't retry to load the shader again. We'll
    // want a new modification before doing so.
    version_ = new_version;

    std::unique_ptr<sf::Shader> new_shader = std::make_unique<sf::Shader>();
    if (new_shader->loadFromFile(kShaderPath, sf::Shader::Fragment)) {
      shader_.swap(new_shader);
      LOG(INFO) << "Reloaded shader";
    } else {
      LOG(WARNING) << "Unable to load shader file " << kShaderPath;
    }
  }
}

void ModShader::Render() {
  MaybeReloadShader();
  if (!shader_) {
    return;
  }

  shader_->setUniform("texture", sf::Shader::CurrentTexture);
  texture_->clear();
  texture_->draw(sf::Sprite(ctx_.CurrentTexture()->getTexture()),
                 shader_.get());
  ctx_.CurrentLayer()->SwapTexture(texture_);
}

} // namespace soir
