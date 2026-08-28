#include "inst/midi_backend.hh"

#include <gtest/gtest.h>

#include <libremidi/libremidi.hpp>

#include <utility>
#include <vector>

#include "inst/external.hh"

namespace soir {
namespace inst {

// When MidiBackendAvailable() reports the backend as usable,
// constructing and destroying libremidi backend objects must be safe.
// A regression that reports the backend as available while the ALSA
// sequencer is inaccessible would crash the observer constructor or the
// midi_out destructor here.
TEST(MidiBackendTest, ConstructionSafeWhenAvailable) {
  if (!MidiBackendAvailable()) {
    GTEST_SKIP() << "MIDI backend unavailable on this system";
  }
  {
    libremidi::midi_out midi_out;
    libremidi::observer observer{};
  }
}

// GetMidiDevices must return OkStatus without crashing whether or not
// the MIDI backend is usable.
TEST(ExternalTest, GetMidiDevices) {
  std::vector<std::pair<int, std::string>> devices;
  auto status = External::GetMidiDevices(&devices);
  EXPECT_TRUE(status.ok());
}

}  // namespace inst
}  // namespace soir
