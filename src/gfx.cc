#include "gfx.h"
#include "mods/debug.h"
#include "mods/text.h"
#include "soir.h"

namespace soir {

Callback::Callback(std::function<void(const MidiMessage &)> func)
    : func_(func) {}

void Callback::SetId(const std::string &id) { id_ = id; }

void Callback::Call(const MidiMessage &msg) { func_(msg); }

bool Callback::operator==(const Callback &rhs) const { return rhs.id_ == id_; }

Mod::~Mod() {
  for (auto &callback : callbacks_) {
    ctx_.Router()->ClearCallback(callback);
  }
}

Status Mod::BindCallback(const std::string &mnemo, const Callback &cb) {
  const Status status = ctx_.Router()->BindCallback(mnemo, cb);

  if (status == StatusCode::OK) {
    callbacks_.push_back(cb);
  }

  return status;
}

StatusOr<std::unique_ptr<Mod>> Mod::MakeMod(Context &ctx,
                                            const std::string &type) {
  if (type == "text") {
    return {std::make_unique<ModText>(ctx)};
  }
  if (type == "debug") {
    return {std::make_unique<ModDebug>(ctx)};
  }

  RETURN_ERROR(StatusCode::UNKNOWN_MOD_TYPE,
               "Unrecognized mode type, type='" << type << "'");
}

void Layer::AppendMod(std::unique_ptr<Mod> mod) {
  mods_.emplace_back(std::move(mod));
}

void Layer::Render() {
  for (auto &mod : mods_) {
    mod->Render();
  }
}

} // namespace soir
