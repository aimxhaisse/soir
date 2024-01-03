#pragma once

#include <absl/status/status.h>
#include <vorbis/vorbisenc.h>

#include "common.hh"
#include "common/config.hh"

#include "audio_buffer.hh"

namespace maethstro {
namespace soir {

// Callback for writing encoded data.
using Writer = std::function<bool(unsigned char*, std::size_t)>;

// Vorbis encoder for streaming audio.
//
// This is based on API overview from https://xiph.org/vorbis/doc/libvorbis/overview.html
class VorbisEncoder {
 public:
  explicit VorbisEncoder(Writer& writer);
  ~VorbisEncoder();

  absl::Status Init(const common::Config& config);

  // If this returns cancel it means the connection was reset by the
  // client. Caller should properly destroy stop the Vorbis encoder
  // which will try to send a valid EOF to the client (ignoring
  // errors).
  absl::Status Encode(const AudioBuffer& buffer);
  absl::Status EndOfStream();

 private:
  void Reset();
  void WriteSamples(int);

  Writer& writer_;
  int quality_ = 100;

  ogg_stream_state os_;
  ogg_page og_;
  ogg_packet op_;
  vorbis_info vi_;
  vorbis_comment vc_;
  vorbis_dsp_state vd_;
  vorbis_block vb_;
};

}  // namespace soir
}  // namespace maethstro
