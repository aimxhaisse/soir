#include <sstream>

#include "mods/debug.h"
#include "soir.h"

namespace soir {

ModDebug::ModDebug(Context &ctx) : Mod(ctx), current_fps_(0), elapsed_fps_(0) {}

Status ModDebug::InitFps(const Config &config) {
  fps_clock_.restart();

  fps_text_.setFont(font_);
  fps_text_.setCharacterSize(12);
  fps_text_.setFillColor(sf::Color(0x00, 0x3B, 0x00, 0xFF));
  fps_text_.setStyle(sf::Text::Bold);
  fps_text_.setPosition(sf::Vector2f(10, 10));

  return StatusCode::OK;
}

Status ModDebug::Init(const Config &config) {
  constexpr const char *font_file = "etc/fonts/pixel-operator.ttf";
  if (!font_.loadFromFile(font_file)) {
    RETURN_ERROR(StatusCode::INVALID_FONT_FILE,
                 "Unable to load font, font_file=" << font_file);
  }

  RETURN_IF_ERROR(InitFps(config));

  return StatusCode::OK;
}

void ModDebug::RenderFps() {
  ++elapsed_fps_;

  const int elapsed_ms = fps_clock_.getElapsedTime().asMilliseconds();
  if (elapsed_ms >= 1000) {
    // This looks like a weird implementation, we only refresh the FPS
    // count every second on the screen so that it's easier to read than
    // a millisecond-precise FPS number that changes all the time.
    current_fps_ = (1000 * elapsed_fps_) / elapsed_ms;

    std::stringstream os;
    os << current_fps_ << " FPS";
    fps_text_.setString(os.str());

    elapsed_fps_ = 0;
    fps_clock_.restart();
  }

  ctx_.CurrentTexture()->draw(fps_text_);
}

void ModDebug::Render() { RenderFps(); }

} // namespace soir
