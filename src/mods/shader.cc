#include <istream>

#include <glog/logging.h>

#include "shader.h"
#include "soir.h"

namespace soir {

constexpr const char *kShaderDir = "etc/shaders/";
constexpr const char *kShaderUniforms = "etc/shaders/uniforms.vert";
constexpr const char *kShaderCommon = "etc/shaders/common.vert";

namespace {

std::string FileToString(const std::string &path) {
  std::ifstream ifs(path);
  return std::string((std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()));
}

} // namespace

ModShader::ModShader(Context &ctx) : Mod(ctx) {}

Status ModShader::Init(const Config &config) {
  if (!sf::Shader::isAvailable()) {
    RETURN_ERROR(INTERNAL_GFX_ERROR,
                 "Shaders aren't supported on this hardware");
  }

  for (const auto &path : {kShaderUniforms, kShaderCommon}) {
    const std::string contents = FileToString(path);
    if (contents.empty()) {
      RETURN_ERROR(INVALID_SHADER_FILE,
                   "Unable to find shader file '" << path << "'.");
    }
    glsl_defaults_ += contents;
  }

  path_ = std::string(kShaderDir) + config.Get<std::string>("params.shader");

  MaybeReloadShader();

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
      boost::filesystem::last_write_time(boost::filesystem::path(path_));
  if (new_version != version_) {
    // Even if we fail to load it, we want to override the version
    // here so that we don't retry to load the shader again. We'll
    // want a new modification before doing so.
    version_ = new_version;

    std::unique_ptr<sf::Shader> new_shader = std::make_unique<sf::Shader>();
    if (new_shader->loadFromMemory(glsl_defaults_ + FileToString(path_),
                                   sf::Shader::Fragment)) {
      shader_.swap(new_shader);
      LOG(INFO) << "Reloaded shader " << path_;
    } else {
      LOG(WARNING) << "Unable to load shader file " << path_;
    }
  }
}

void ModShader::Render() {
  MaybeReloadShader();

  texture_->clear();

  if (shader_) {
    shader_->setUniform("texture", sf::Shader::CurrentTexture);

    const sf::Time time = ctx_.Clock().getElapsedTime();
    shader_->setUniform("time_s", time.asSeconds());
    shader_->setUniform("time_ms", time.asMilliseconds());

    shader_->setUniform(
        "resolution", sf::Glsl::Vec2(static_cast<float>(ctx_.BufferWidth()),
                                     static_cast<float>(ctx_.BufferHeight())));

    texture_->draw(sf::Sprite(ctx_.CurrentTexture()->getTexture()),
                   shader_.get());
  }

  ctx_.CurrentLayer()->SwapTexture(texture_);
}

} // namespace soir
