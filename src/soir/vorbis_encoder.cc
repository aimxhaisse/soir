#include <absl/log/log.h>

#include "vorbis_encoder.hh"

namespace maethstro {
namespace soir {

VorbisEncoder::VorbisEncoder(Writer& writer) : writer_(writer) {}

VorbisEncoder::~VorbisEncoder() {
  Reset();
}

void VorbisEncoder::Reset() {
  ogg_stream_clear(&os_);
  vorbis_block_clear(&vb_);
  vorbis_dsp_clear(&vd_);
  vorbis_comment_clear(&vc_);

  vorbis_info_clear(&vi_);
}

absl::Status VorbisEncoder::Init(const common::Config& config) {
  quality_ = config.Get<int>("soir.stream.quality");

  vorbis_info_init(&vi_);

  if (vorbis_encode_init_vbr(&vi_, kNumChannels, kSampleRate, quality_)) {
    LOG(WARNING) << "Unable to initialize Vorbis encoder";
    return absl::InternalError("Unable to initialize Vorbis encoder");
  }

  vorbis_comment_init(&vc_);

  vorbis_comment_add_tag(&vc_, "title", "Maethstro L I V E");
  vorbis_analysis_init(&vd_, &vi_);
  vorbis_block_init(&vd_, &vb_);

  ogg_stream_init(&os_, rand());

  ogg_packet header, header_comm, header_code;

  vorbis_analysis_headerout(&vd_, &vc_, &header, &header_comm, &header_code);

  ogg_stream_packetin(&os_, &header);
  ogg_stream_packetin(&os_, &header_comm);
  ogg_stream_packetin(&os_, &header_code);

  for (;;) {
    if (ogg_stream_flush(&os_, &og_) == 0) {
      break;
    }
    if (!writer_(og_.header, static_cast<std::size_t>(og_.header_len))) {
      return absl::CancelledError("Connection reset by peer");
    }
    if (!writer_(og_.body, static_cast<std::size_t>(og_.body_len))) {
      return absl::CancelledError("Connection reset by peer");
    }
  }

  return absl::OkStatus();
}

absl::Status VorbisEncoder::Encode(const AudioBuffer& buffer) {
  return absl::OkStatus();
}

absl::Status VorbisEncoder::EndOfStream() {
  return absl::OkStatus();
}

}  // namespace soir
}  // namespace maethstro
