#include <absl/log/log.h>
#include <gtest/gtest.h>
#include <thread>

#include "matin.hh"
#include "midi/midi.hh"

namespace maethstro {
namespace matin {
namespace {

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

class MatinTest : public testing::Test {
 protected:
  void SetUp() override {
    auto config_or = Config::LoadFromString(config_yaml);
    ASSERT_TRUE(config_or.ok());
    config_ = std::move(*config_or);

    midi_ = std::make_unique<midi::Midi>();
    ASSERT_TRUE(midi_->Init(*config_).ok());
    ASSERT_TRUE(midi_->Start().ok());
  }

  void TearDown() override {
    ASSERT_TRUE(midi_->Stop().ok());
    midi_.reset();
    config_.reset();
  }

  std::unique_ptr<midi::Midi> midi_;
  std::unique_ptr<Config> config_;
};

TEST_F(MatinTest, Init) {
  Matin matin;

  ASSERT_TRUE(matin.Init(**config_).ok());
  ASSERT_TRUE(matin.Start().ok());
  ASSERT_TRUE(matin.Stop().ok());
}

}  // namespace
}  // namespace matin
}  // namespace maethstro
