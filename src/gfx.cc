#include <glog/logging.h>

#include "gfx.h"
#include "mods/debug.h"
#include "mods/noise.h"
#include "mods/text.h"
#include "soir.h"

namespace soir {

Callback::Callback(std::function<void(const MidiMessage &)> func)
    : func_(func) {}

void Callback::SetId(const std::string &id) { id_ = id; }

void Callback::Call(const MidiMessage &msg) { func_(msg); }

bool Callback::operator==(const Callback &rhs) const { return rhs.id_ == id_; }

Mod::Mod(Context &ctx) : ctx_(ctx) {}

Mod::~Mod() {
  for (auto &it : callbacks_) {
    ctx_.Router()->ClearCallback(it.second);
  }
}

void Mod::RegisterCallback(const std::string &callback_name,
                           const Callback &cb) {
  Callback callback = cb;

  // Set the Mod address as part of the dentifier of the callback,
  // sucks but is good-enough since we cleanup everything in
  // destructor.
  std::stringstream ss;
  ss << this;
  callback.SetId(ss.str() + '.' + callback_name);

  callbacks_[callback_name] = callback;
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
  if (type == "noise") {
    return {std::make_unique<ModNoise>(ctx)};
  }

  RETURN_ERROR(StatusCode::UNKNOWN_MOD_TYPE,
               "Unrecognized mode type, type='" << type << "'");
}

Layer::Layer(Context &ctx) : ctx_(ctx) {}

Status Layer::Init() {
  texture_ = std::make_unique<sf::Texture>();
  texture_->create(ctx_.Width(), ctx_.Height());
  return StatusCode::OK;
}

void Layer::AppendMod(std::unique_ptr<Mod> mod) {
  mods_.emplace_back(std::move(mod));
}

void Layer::Render() {
  sf::Sprite sprite;

  ctx_.current_txt_ = texture_.get();
  ctx_.current_sprite_ = &sprite;

  sprite.setTexture(*ctx_.CurrentTexture());

  for (auto &mod : mods_) {
    mod->Render();
  }

  ctx_.Window()->draw(sprite);

  ctx_.current_txt_ = nullptr;
  ctx_.current_sprite_ = nullptr;
}

} // namespace soir
