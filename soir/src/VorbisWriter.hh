#pragma once

#include <JuceHeader.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include <map>

#include "Status.hh"

// Based on JUCE, but supports comments in the middle of the stream,
// also only limited to what we actually need.

namespace maethstro {

struct VorbisWriter : public juce::AudioFormatWriter {
  explicit VorbisWriter(juce::OutputStream* out);
  ~VorbisWriter();

  Status Reset(const std::string& metadata);

  bool write(const int** samplesToWrite, int numSamples);
  void writeSamples(int numSamples);
  void addMetadata(const juce::StringPairArray& metadata, const char* name,
                   const char* vorbisName);

 private:
  void Cleanup();

  bool init_;

  ogg_stream_state os_;
  ogg_page og_;
  ogg_packet op_;
  vorbis_info vi_;
  vorbis_comment vc_;
  vorbis_dsp_state vd_;
  vorbis_block vb_;
};

}  // namespace maethstro
