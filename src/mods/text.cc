#include "mods/text.h"
#include "soir.h"

namespace soir {

Status ModText::Init(Context &ctx, const Config &config) {
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

void ModText::Render(Context &ctx) { ctx.Window()->draw(text_); }

} // namespace soir
