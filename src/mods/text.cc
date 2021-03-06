#include <glog/logging.h>

#include "mods/text.h"
#include "soir.h"

using namespace std::placeholders;

namespace soir {

ModText::ModText(Context &ctx) : Mod(ctx) {}

Status ModText::Init(const Config &config) {
  RegisterCallback("shiftLetters",
                   Callback(std::bind(&ModText::ShiftLetters, this, _1)));
  RETURN_IF_ERROR(BindCallbacks(config));

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

  CenterText();

  return StatusCode::OK;
}

void ModText::CenterText() {
  const sf::FloatRect bounds = text_.getLocalBounds();
  text_.setOrigin(bounds.left + bounds.width / 2.0f,
                  bounds.top + bounds.height / 2.0f);
  text_.setPosition(
      sf::Vector2f(ctx_.BufferWidth() / 2.0f, ctx_.BufferHeight() / 2.0f));
}

void ModText::Render() { ctx_.CurrentTexture()->draw(text_); }

void ModText::ShiftLetters(const MidiMessage &) {
  std::string text = text_.getString();
  if (!text.empty()) {
    const char first = text[0];
    text.erase(0, 1);
    text.push_back(first);
    text_.setString(text);
  }
}

} // namespace soir
