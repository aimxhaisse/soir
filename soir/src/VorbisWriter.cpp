#include "VorbisWriter.hh"

#include <glog/logging.h>

#include "Constants.hh"

namespace maethstro {

VorbisWriter::VorbisWriter(juce::OutputStream* out)
    : juce::AudioFormatWriter(out, "Ogg-Vorbis file", kSampleRate, kStereo,
                              kBitsPerSample) {}

Status VorbisWriter::Reset(const std::string& metadata) {
  if (init_) {
    LOG(INFO) << "cleaning maethstro fine-tuned vorbis writer";
    Cleanup();
  }

  LOG(INFO) << "initializing maethstro fine-tuned vorbis writer";

  vorbis_info_init(&vi_);

  if (vorbis_encode_init_vbr(&vi_, kStereo, kSampleRate, kVorbisQuality)) {
    LOG(WARNING) << "unable to initialize Vorbis encoder";
    return StatusCode::INTERNAL_ERROR;
  }

  vorbis_comment_init(&vc_);

  // Only Title is supported by LiquidSoap.
  vorbis_comment_add_tag(&vc_, "title", metadata.data());

  vorbis_analysis_init(&vd_, &vi_);
  vorbis_block_init(&vd_, &vb_);

  ogg_stream_init(&os_, juce::Random::getSystemRandom().nextInt());

  ogg_packet header, header_comm, header_code;
  vorbis_analysis_headerout(&vd_, &vc_, &header, &header_comm, &header_code);

  ogg_stream_packetin(&os_, &header);
  ogg_stream_packetin(&os_, &header_comm);
  ogg_stream_packetin(&os_, &header_code);

  for (;;) {
    if (ogg_stream_flush(&os_, &og_) == 0) {
      break;
    }

    output->write(og_.header, static_cast<std::size_t>(og_.header_len));
    output->write(og_.body, static_cast<std::size_t>(og_.body_len));
  }

  init_ = true;

  return StatusCode::OK;
}

void VorbisWriter::Cleanup() {
  // write a zero-length packet to show ogg that we're finished
  writeSamples(0);

  ogg_stream_clear(&os_);
  vorbis_block_clear(&vb_);
  vorbis_dsp_clear(&vd_);
  vorbis_comment_clear(&vc_);

  vorbis_info_clear(&vi_);
  output->flush();
}

VorbisWriter::~VorbisWriter() { Cleanup(); }

bool VorbisWriter::write(const int** samplesToWrite, int numSamples) {
  if (numSamples > 0) {
    const double gain = 1.0 / 0x80000000u;
    float** const vorbisBuffer = vorbis_analysis_buffer(&vd_, numSamples);

    for (int i = (int)numChannels; --i >= 0;) {
      if (auto* dst = vorbisBuffer[i]) {
        if (const int* src = samplesToWrite[i]) {
          for (int j = 0; j < numSamples; ++j) {
            dst[j] = (float)(src[j] * gain);
          }
        }
      }
    }
  }

  writeSamples(numSamples);

  return true;
}

void VorbisWriter::writeSamples(int numSamples) {
  vorbis_analysis_wrote(&vd_, numSamples);

  while (vorbis_analysis_blockout(&vd_, &vb_) == 1) {
    vorbis_analysis(&vb_, nullptr);
    vorbis_bitrate_addblock(&vb_);

    while (vorbis_bitrate_flushpacket(&vd_, &op_)) {
      ogg_stream_packetin(&os_, &op_);

      while (true) {
        if (ogg_stream_pageout(&os_, &og_) == 0) {
          break;
        }

        output->write(og_.header, static_cast<std::size_t>(og_.header_len));
        output->write(og_.body, static_cast<std::size_t>(og_.body_len));

        if (ogg_page_eos(&og_)) {
          break;
        }
      }
    }
  }
}

void VorbisWriter::addMetadata(const juce::StringPairArray&, const char*,
                               const char*) {}

}  // namespace maethstro
