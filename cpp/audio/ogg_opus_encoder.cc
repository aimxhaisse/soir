#include "audio/ogg_opus_encoder.hh"

#include <cstring>
#include <random>

#include "absl/log/log.h"
#include "ogg/ogg.h"
#include "opus.h"

namespace soir {
namespace audio {

namespace {

void WriteLE16(uint8_t* buf, uint16_t val) {
  buf[0] = static_cast<uint8_t>(val & 0xFF);
  buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
}

void WriteLE32(uint8_t* buf, uint32_t val) {
  buf[0] = static_cast<uint8_t>(val & 0xFF);
  buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
  buf[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
  buf[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
}

}  // namespace

void OpusEncoderDeleter::operator()(OpusEncoder* e) const {
  if (e != nullptr) {
    opus_encoder_destroy(e);
  }
}

OggOpusEncoder::OggOpusEncoder() = default;

OggOpusEncoder::~OggOpusEncoder() {
  if (initialized_) {
    ogg_stream_clear(&ogg_stream_);
  }
}

absl::Status OggOpusEncoder::Init(int sample_rate, int channels, int bitrate) {
  sample_rate_ = sample_rate;
  channels_ = channels;

  // Opus always operates at 48kHz internally for music.
  int err;
  encoder_.reset(
      opus_encoder_create(48000, channels, OPUS_APPLICATION_AUDIO, &err));
  if (err != OPUS_OK) {
    encoder_.reset();
    return absl::InternalError("Failed to create Opus encoder: " +
                               std::string(opus_strerror(err)));
  }

  err = opus_encoder_ctl(encoder_.get(), OPUS_SET_BITRATE(bitrate));
  if (err != OPUS_OK) {
    return absl::InternalError("Failed to set Opus bitrate: " +
                               std::string(opus_strerror(err)));
  }

  // Initialize Ogg stream with a random serial number.
  std::random_device rd;
  int serial = static_cast<int>(rd());

  if (ogg_stream_init(&ogg_stream_, serial) != 0) {
    return absl::InternalError("Failed to initialize Ogg stream");
  }

  auto status = GenerateHeaders();
  if (!status.ok()) {
    return status;
  }

  initialized_ = true;

  LOG(INFO) << "Ogg/Opus encoder initialized: " << sample_rate << "Hz, "
            << channels << " channels, " << bitrate << " bps";

  return absl::OkStatus();
}

absl::Status OggOpusEncoder::GenerateHeaders() {
  // OpusHead header (19 bytes for channel mapping 0).
  // See https://www.rfc-editor.org/rfc/rfc7845#section-5.1
  uint8_t opus_head[19];
  memcpy(opus_head, "OpusHead", 8);
  opus_head[8] = 1;  // version
  opus_head[9] = static_cast<uint8_t>(channels_);
  WriteLE16(opus_head + 10, 3840);  // pre-skip (80ms at 48kHz)
  WriteLE32(opus_head + 12, static_cast<uint32_t>(sample_rate_));
  WriteLE16(opus_head + 16, 0);  // output gain
  opus_head[18] = 0;             // channel mapping family

  ogg_packet header_packet;
  header_packet.packet = opus_head;
  header_packet.bytes = 19;
  header_packet.b_o_s = 1;
  header_packet.e_o_s = 0;
  header_packet.granulepos = 0;
  header_packet.packetno = packetno_++;

  ogg_stream_packetin(&ogg_stream_, &header_packet);

  auto status = WriteOggPages(&header_pages_, true);
  if (!status.ok()) {
    return status;
  }

  // OpusTags header.
  // See https://www.rfc-editor.org/rfc/rfc7845#section-5.2
  const char* vendor = "soir";
  uint32_t vendor_len = 4;
  uint32_t comment_count = 0;

  std::vector<uint8_t> opus_tags(8 + 4 + vendor_len + 4);
  memcpy(opus_tags.data(), "OpusTags", 8);
  WriteLE32(opus_tags.data() + 8, vendor_len);
  memcpy(opus_tags.data() + 12, vendor, vendor_len);
  WriteLE32(opus_tags.data() + 16, comment_count);

  ogg_packet tags_packet;
  tags_packet.packet = opus_tags.data();
  tags_packet.bytes = static_cast<long>(opus_tags.size());
  tags_packet.b_o_s = 0;
  tags_packet.e_o_s = 0;
  tags_packet.granulepos = 0;
  tags_packet.packetno = packetno_++;

  ogg_stream_packetin(&ogg_stream_, &tags_packet);

  return WriteOggPages(&header_pages_, true);
}

absl::Status OggOpusEncoder::Encode(const float* pcm, int frame_size,
                                    std::vector<uint8_t>* output) {
  if (!initialized_) {
    return absl::FailedPreconditionError("Encoder not initialized");
  }

  uint8_t packet_buffer[4000];
  int encoded_bytes =
      opus_encode_float(encoder_.get(), pcm, frame_size, packet_buffer,
                        sizeof(packet_buffer));
  if (encoded_bytes < 0) {
    return absl::InternalError("Opus encoding failed: " +
                               std::string(opus_strerror(encoded_bytes)));
  }

  granulepos_ += frame_size;

  ogg_packet op;
  op.packet = packet_buffer;
  op.bytes = encoded_bytes;
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = granulepos_;
  op.packetno = packetno_++;

  ogg_stream_packetin(&ogg_stream_, &op);

  return WriteOggPages(output, false);
}

absl::Status OggOpusEncoder::Flush(std::vector<uint8_t>* output) {
  if (!initialized_) {
    return absl::FailedPreconditionError("Encoder not initialized");
  }

  ogg_packet op;
  op.packet = nullptr;
  op.bytes = 0;
  op.b_o_s = 0;
  op.e_o_s = 1;
  op.granulepos = granulepos_;
  op.packetno = packetno_++;

  ogg_stream_packetin(&ogg_stream_, &op);

  return WriteOggPages(output, true);
}

const std::vector<uint8_t>& OggOpusEncoder::GetHeaderPages() const {
  return header_pages_;
}

absl::Status OggOpusEncoder::WriteOggPages(std::vector<uint8_t>* output,
                                           bool flush) {
  ogg_page page;

  while (true) {
    int result;
    if (flush) {
      result = ogg_stream_flush(&ogg_stream_, &page);
    } else {
      result = ogg_stream_pageout(&ogg_stream_, &page);
    }

    if (result == 0) {
      break;
    }

    output->insert(output->end(), page.header,
                   page.header + page.header_len);
    output->insert(output->end(), page.body, page.body + page.body_len);
  }

  return absl::OkStatus();
}

}  // namespace audio
}  // namespace soir
