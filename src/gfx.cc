#include "gfx.h"

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

Status ModText::Init(const Config &config) {
  constexpr const char *font_file = "etc/fonts/pixel-operator.ttf";
  if (!font_.loadFromFile(font_file)) {
    RETURN_ERROR(StatusCode::INVALID_FONT_FILE,
                 "Unable to load font, font_file=" << font_file);
  }

  const std::string text = config.Get<std::string>("params.text");
  text_.setFont(font_);
  text_.setString(text);
  text_.setCharacterSize(24);
  text_.setFillColor(sf::Color::Green);
  text_.setStyle(sf::Text::Bold);

  return StatusCode::OK;
}

void ModText::Render(sf::RenderWindow &window) { window.draw(text_); }

Status ModDebug::Init(const Config &config) { return StatusCode::OK; }

void ModDebug::Render(sf::RenderWindow &window) {}

void Layer::AppendMod(std::unique_ptr<Mod> mod) {
  mods_.emplace_back(std::move(mod));
}

void Layer::Render(sf::RenderWindow &window) {
  for (auto &mod : mods_) {
    mod->Render(window);
  }
}

} // namespace soir
