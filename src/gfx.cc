#include "gfx.h"

namespace soir {

StatusOr<std::unique_ptr<Mod>> Mod::MakeMod(const std::string &type) {
  if (type == "text") {
    return {std::make_unique<ModText>()};
  }

  RETURN_ERROR(StatusCode::UNKNOWN_MOD_TYPE,
               "Unrecognized mode type, type='" << type << "'");
}

Status ModText::Init(const Config &config) { return StatusCode::OK; }

void Layer::AppendMod(std::unique_ptr<Mod> mod) {
  mods_.emplace_back(std::move(mod));
}

} // namespace soir
