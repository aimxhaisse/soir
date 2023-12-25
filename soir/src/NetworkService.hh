#pragma once

#include <JuceHeader.h>

// For some obscure reason, this one is not available in juce_audio_basics
// despite living there. It is imported in... another module, but only on some
// arch.
#include <juce_audio_basics/midi/juce_MidiDataConcatenator.h>

#include <atomic>
#include <memory>

#include "Config.hh"
#include "IFaces.hh"

namespace maethstro {

struct NetworkService {
  static StatusOr<std::unique_ptr<NetworkService>> InitFromConfig(
      const Config& config, IMidiConsumer* consumer);

  void Stop();
  Status Loop(std::atomic<bool>& stop);

  // Required by the MIDI data concatenator from juce. This is kind-of sad as it
  // does not handle any sort of error handling. Nor can we easily reset the
  // stream if we somehow get out-of-sync.
  //
  // No idea why they enforce the partial sysex.
  void handleIncomingMidiMessage(void*, juce::MidiMessage msg);
  void handlePartialSysexMessage(void*, uint8*, int, double) {}

 private:
  juce::StreamingSocket socket_;
  std::unique_ptr<juce::StreamingSocket> client_;
  std::unique_ptr<juce::MidiDataConcatenator> midiParser_;
  IMidiConsumer* midiConsumer_;
};

}  // namespace maethstro
