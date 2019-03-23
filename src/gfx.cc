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

Mod::Mod(Context &ctx) : ctx_(ctx) {
  // Set the Mod address as part of the dentifier of the callback,
  // sucks but is good-enough since we cleanup everything in
  // destructor. Multiple callbacks from the same callback will have
  // the same identifier. That's fine.
  std::stringstream ss;
  ss << this;
  id_ = ss.str();
}

Mod::~Mod() {
  Callback cb;
  cb.SetId(id_);
  ctx_.Router()->ClearCallback(cb);
}

Status Mod::BindCallback(const MidiMnemo &mnemo, Callback cb) {
  cb.SetId(id_);
  return ctx_.Router()->BindCallback(mnemo, cb);
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
