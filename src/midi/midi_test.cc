#include <absl/log/log.h>
#include <gtest/gtest.h>

#include "midi.hh"

namespace maethstro {
namespace midi {
namespace {

class MidiTest : public testing::Test {};

const char* config_yaml = R"eof(
# Maethstro
#  _       ___  __     __  _____ 
# | |     |_ _| \ \   / / | ____|
# | |      | |   \ \ / /  |  _|  
# | |___   | |    \ V /   | |___ 
# |_____| |___|    \_/    |_____|
#
# Standalone edition.

# MATIN configuration.
matin:
  # Name of the user.
  user: mxs

  # Watches all Python files in this directory and send them live to
  # Midi upon change.
  directory: live

  # Where to find the Maethstro Midi service. This can be remote in
  # case of collaborations but in the standalone mode it uses a local
  # connection.
  midi:
    grpc:
      host: 127.0.0.1
      port: 7000

# MIDI configuration.
midi:
  initial_bpm: 120
  grpc:
    host: 127.0.0.1
    port: 7000

# SOIR configuration.
soir:
  grpc:
    host: 127.0.0.1
    port: 7001
)eof";

TEST_F(MidiTest, Init) {
  auto config = common::Config::LoadFromString(config_yaml);

  ASSERT_TRUE(config.ok());

  Midi midi;

  ASSERT_TRUE(midi.Init(**config).ok());
  ASSERT_TRUE(midi.Start().ok());
  ASSERT_TRUE(midi.Stop().ok());
}

}  // namespace
}  // namespace midi
}  // namespace maethstro
