#include <absl/log/log.h>
#include <gtest/gtest.h>
#include <thread>

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
  # Name of the user, will likely be later moved to a Unix username or
  # so.
  username: mxs

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
  auto config = Config::LoadFromString(config_yaml);

  ASSERT_TRUE(config.ok());

  Midi midi;

  ASSERT_TRUE(midi.Init(**config).ok());

  std::thread midi_thread([&midi]() {
    auto status = midi.Run();
    if (!status.ok()) {
      LOG(ERROR) << "Unable to run midi: " << status;
      exit(1);
    }
  });

  ASSERT_TRUE(midi.Stop().ok());
  ASSERT_TRUE(midi.Wait().ok());

  midi_thread.join();
}

}  // namespace
}  // namespace midi
}  // namespace maethstro
