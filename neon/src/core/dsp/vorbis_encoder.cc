#include <absl/log/log.h>

#include "vorbis_encoder.hh"

namespace neon {
namespace dsp {

VorbisEncoder::VorbisEncoder() {}

VorbisEncoder::~VorbisEncoder() {}

absl::Status VorbisEncoder::Init(Writer& writer) {
  LOG(INFO) << "Initializing Vorbis encoder";

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
    if (!writer(og_.header, static_cast<std::size_t>(og_.header_len))) {
      return absl::CancelledError("Connection reset by peer");
    }
    if (!writer(og_.body, static_cast<std::size_t>(og_.body_len))) {
      return absl::CancelledError("Connection reset by peer");
    }
  }

  return absl::OkStatus();
}

absl::Status VorbisEncoder::Encode(AudioBuffer& ab, Writer& writer) {
  float* left = ab.GetChannel(kLeftChannel);
  float* right = ab.GetChannel(kRightChannel);

  std::size_t samples = ab.Size();

  float** buffer = vorbis_analysis_buffer(&vd_, samples);
  memcpy(buffer[0], left, sizeof(float) * samples);
  memcpy(buffer[1], right, sizeof(float) * samples);

  vorbis_analysis_wrote(&vd_, samples);

  while (vorbis_analysis_blockout(&vd_, &vb_) == 1) {
    vorbis_analysis(&vb_, nullptr);
    vorbis_bitrate_addblock(&vb_);

    while (vorbis_bitrate_flushpacket(&vd_, &op_)) {
      ogg_stream_packetin(&os_, &op_);

      while (ogg_stream_pageout(&os_, &og_) == 1) {
        if (!writer(og_.header, static_cast<std::size_t>(og_.header_len))) {
          return absl::CancelledError("Connection reset by peer");
        }
        if (!writer(og_.body, static_cast<std::size_t>(og_.body_len))) {
          return absl::CancelledError("Connection reset by peer");
        }
      }
    }
  }

  return absl::OkStatus();
}

}  // namespace dsp
}  // namespace neon
