#include <glog/logging.h>

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

void Mod::RegisterCallback(const std::string &callback_name,
                           const Callback &cb) {
  callbacks_[callback_name] = cb;
}

Status Mod::BindCallbacks(const Config &mod_config) {
  const auto bindings =
      mod_config.Get<std::map<std::string, std::vector<std::string>>>(
          "bindings");

  for (const auto &it : bindings) {
    const std::string &mnemo = it.first;
    for (const std::string &callback_name : it.second) {
      if (callbacks_.find(callback_name) == callbacks_.end()) {
        RETURN_ERROR(StatusCode::INVALID_CONFIG_FILE,
                     "Unable to bind MIDI mnemo=" << mnemo << " to callback="
                                                  << callback_name
                                                  << " (doesn't exist)");
      }
      RETURN_IF_ERROR(
          ctx_.Router()->BindCallback(mnemo, callbacks_[callback_name]));
      LOG(INFO) << "Callback " << callback_name << " now listening to "
                << mnemo;
    }
  }

  return StatusCode::OK;
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
