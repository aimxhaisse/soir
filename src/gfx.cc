#include "gfx.h"
#include "mods/debug.h"
#include "mods/text.h"
#include "soir.h"

namespace soir {

StatusOr<std::unique_ptr<Mod>> Mod::MakeMod(const std::string &type) {
  if (type == "text") {
    return {std::make_unique<ModText>()};
  }
  if (type == "debug") {
    return {std::make_unique<ModDebug>()};
  }

  RETURN_ERROR(StatusCode::UNKNOWN_MOD_TYPE,
               "Unrecognized mode type, type='" << type << "'");
}

void Layer::AppendMod(std::unique_ptr<Mod> mod) {
  mods_.emplace_back(std::move(mod));
}

void Layer::Render(Context &ctx) {
  for (auto &mod : mods_) {
    mod->Render(ctx);
  }
}

} // namespace soir
