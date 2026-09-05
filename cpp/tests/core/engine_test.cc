#include "core/engine.hh"

#include <absl/log/log_entry.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>
#include <gtest/gtest.h>

#include <mutex>
#include <string>
#include <vector>

namespace soir {
namespace {

// Test config with audio output disabled
constexpr const char* kTestConfig = R"({
  "dsp": {
    "enable_output": false,
    "enable_streaming": false,
    "streaming_host": "localhost",
    "streaming_port": 5001,
    "sample_directory": "/tmp",
    "sample_packs": []
  },
  "vst": {
    "scan_at_startup": false
  }
})";

// Captures log messages so tests can assert on their ordering.
class CapturingLogSink : public absl::LogSink {
 public:
  void Send(const absl::LogEntry& entry) override {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back(std::string(entry.text_message_with_newline()));
  }

  size_t Size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return lines_.size();
  }

  std::vector<std::string> LinesSince(size_t from) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result(lines_.begin() + from, lines_.end());
    return result;
  }

 private:
  std::mutex mutex_;
  std::vector<std::string> lines_;
};

}  // namespace

TEST(EngineTest, Construction) {
  Engine engine;
  // Basic construction should succeed
  EXPECT_TRUE(true);
}

TEST(EngineTest, Initialization) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto status = engine.Init(config);
  EXPECT_TRUE(status.ok()) << status.message();
}

TEST(EngineTest, StartStop) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok()) << init_status.message();

  auto start_status = engine.Start();
  EXPECT_TRUE(start_status.ok()) << start_status.message();

  auto stop_status = engine.Stop();
  EXPECT_TRUE(stop_status.ok()) << stop_status.message();
}

TEST(EngineTest, GetSampleManager) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok());

  SampleManager& manager = engine.GetSampleManager();
  // Should return a valid reference
  EXPECT_TRUE(true);
}

TEST(EngineTest, GetControls) {
  Engine engine;
  utils::Config config(kTestConfig);

  auto init_status = engine.Init(config);
  ASSERT_TRUE(init_status.ok());

  Controls* controls = engine.GetControls();
  EXPECT_NE(controls, nullptr);
}

// Replacing a track must fully destroy the old track (thread joined, VST
// plugins shut down) BEFORE the replacement is initialised. With yabridge
// all instances of the same plugin share a single Wine coprocess, and
// tearing an old instance down while fresh instances of the same plugin are
// already live on that coprocess can crash the coprocess and hang the whole
// session.
TEST(EngineTest, SetupTracksDestroysOldTrackBeforeCreatingReplacement) {
  CapturingLogSink sink;
  absl::AddLogSink(&sink);

  Engine engine;
  utils::Config config(kTestConfig);
  ASSERT_TRUE(engine.Init(config).ok());
  ASSERT_TRUE(engine.Start().ok());

  auto make_settings = [](const std::list<fx::Fx::Settings>& fxs) {
    Track::Settings s;
    s.name_ = "A";
    s.instrument_ = inst::Type::SAMPLER;
    s.extra_ = "{}";
    s.fxs_ = fxs;
    return s;
  };

  auto reverb = [](const char* name) {
    fx::Fx::Settings s;
    s.name_ = name;
    s.type_ = fx::Type::REVERB;
    return s;
  };
  auto chorus = [](const char* name) {
    fx::Fx::Settings s;
    s.name_ = name;
    s.type_ = fx::Type::CHORUS;
    return s;
  };

  // First layout: one FX.
  std::list<Track::Settings> first;
  first.push_back(make_settings({reverb("r")}));
  ASSERT_TRUE(engine.SetupTracks(first).ok());

  // Second layout: two FXs. Adding the new FX prevents a fast update, so
  // the track is replaced with a full rebuild.
  std::list<Track::Settings> second;
  second.push_back(make_settings({reverb("r"), chorus("c")}));
  const size_t before = sink.Size();
  ASSERT_TRUE(engine.SetupTracks(second).ok());

  engine.Stop().IgnoreError();
  absl::RemoveLogSink(&sink);

  // Find the old track's stop and the new track's start in the log lines
  // emitted by the second setup. Use first occurrences: the engine shutdown
  // at the end of the test logs similar lines as well.
  auto lines = sink.LinesSince(before);
  int old_stop = -1;
  int new_start = -1;
  int new_fx_init = -1;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (old_stop == -1 &&
        lines[i].find("Stopping track thread for: A") != std::string::npos) {
      old_stop = static_cast<int>(i);
    }
    if (new_start == -1 &&
        lines[i].find("Starting track thread for: A") != std::string::npos) {
      new_start = static_cast<int>(i);
    }
    if (new_fx_init == -1 &&
        lines[i].find("Initialized FX 'c'") != std::string::npos) {
      new_fx_init = static_cast<int>(i);
    }
  }

  auto dump = [&lines]() {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
      out += std::to_string(i) + ": " + lines[i];
    }
    return out;
  };

  ASSERT_NE(old_stop, -1) << "old track was not stopped\n" << dump();
  ASSERT_NE(new_start, -1) << "new track was not started\n" << dump();
  ASSERT_NE(new_fx_init, -1) << "new FX was not initialised\n" << dump();
  EXPECT_LT(old_stop, new_fx_init)
      << "old track must be destroyed before the replacement is "
         "initialised\n"
      << dump();
  EXPECT_LT(old_stop, new_start)
      << "old track must be stopped before the replacement is "
         "started\n"
      << dump();
}

}  // namespace soir
