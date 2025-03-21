#pragma once

#include <absl/status/status.h>
#include <vorbis/vorbisenc.h>

#include "utils/config.hh"
#include "utils/misc.hh"

#include "core/audio_buffer.hh"
#include "core/dsp.hh"

namespace soir {


// Callback for writing encoded data.
using Writer = std::function<bool(unsigned char*, std::size_t)>;

// Vorbis encoder for streaming audio.
//
// This is based on API overview from https://xiph.org/vorbis/doc/libvorbis/overview.html
class VorbisEncoder {
 public:
  VorbisEncoder();
  ~VorbisEncoder();

  // If this returns cancel it means the connection was reset by the
  // client. Caller should properly destroy stop the Vorbis encoder
  //
  // We don't properly offer a way to nicely stop the stream for now,
  // this could be an addition if we want to be able to store proper
  // recordings or so.
  absl::Status Init(Writer& writer);
  absl::Status Encode(AudioBuffer& buffer, Writer& writer);

 private:
  const float quality_ = kVorbisQuality;

  ogg_stream_state os_;
  ogg_page og_;
  ogg_packet op_;
  vorbis_info vi_;
  vorbis_comment vc_;
  vorbis_dsp_state vd_;
  vorbis_block vb_;
};


}  // namespace soir
