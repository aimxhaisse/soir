#include <glog/logging.h>

#include "mods/text.h"
#include "soir.h"

using namespace std::placeholders;

namespace soir {

ModText::ModText(Context &ctx) : Mod(ctx) {}

Status ModText::Init(const Config &config) {
  RETURN_IF_ERROR(
      BindCallback("Digitakt Elektron MIDI.kick",
                   Callback(std::bind(&ModText::OnEvent, this, _1))));

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

void ModText::Render() { ctx_.Window()->draw(text_); }

void ModText::OnEvent(const MidiMessage &message) {
  LOG(INFO) << "ModText OnEvent called \\o/";
}

} // namespace soir
