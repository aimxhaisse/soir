#ifndef SOIR_MOD_EXP_H
#define SOIR_MOD_EXP_H

#include <boost/filesystem.hpp>

#include "gfx.h"

namespace soir {

class ModShader : public Mod {
public:
  static constexpr const char *kModName = "shader";

  ModShader(Context &ctx);
  Status Init(const Config &config);
  void Render();

  sf::Shader &GetShader();

private:
  void MaybeReloadShader();

  std::unique_ptr<sf::RenderTexture> texture_;
  std::unique_ptr<sf::Shader> shader_;
  std::string path_;
  std::time_t version_;

  std::string glsl_defaults_;
};

} // namespace soir

#endif // SOIR_MOD_EXP_H
