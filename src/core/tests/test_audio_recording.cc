#include <AudioFile.h>
#include <filesystem>
#include <thread>

#include "base.hh"

namespace soir {
namespace core {
namespace test {

class AudioRecordingTest : public CoreTestBase {};

TEST_F(AudioRecordingTest, BasicRecordingFileCreation) {
  const std::string test_file = "/tmp/soir_test_recording_basic.wav";

  // Remove test file if it exists
  std::filesystem::remove(test_file);
  EXPECT_FALSE(std::filesystem::exists(test_file));

  PushCode(R"(
sys.record("/tmp/soir_test_recording_basic.wav")
log("recording_started")
  )");

  // Wait for recording to start
  EXPECT_TRUE(WaitForNotification("recording_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Stop recording by pushing empty code
  PushCode(R"(
log("recording_stopped")
  )");

  EXPECT_TRUE(WaitForNotification("recording_stopped"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Verify file was created
  EXPECT_TRUE(std::filesystem::exists(test_file));

  // Clean up
  std::filesystem::remove(test_file);
}

TEST_F(AudioRecordingTest, ValidWAVFileProperties) {
  const std::string test_file = "/tmp/soir_test_recording_wav.wav";

  // Remove test file if it exists
  std::filesystem::remove(test_file);

  PushCode(R"(
sys.record("/tmp/soir_test_recording_wav.wav")
log("recording_started")
  )");

  EXPECT_TRUE(WaitForNotification("recording_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Stop recording
  PushCode(R"(
log("recording_stopped")
  )");

  EXPECT_TRUE(WaitForNotification("recording_stopped"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Verify file exists and is a valid WAV file
  EXPECT_TRUE(std::filesystem::exists(test_file));

  AudioFile<float> audio_file;
  EXPECT_TRUE(audio_file.load(test_file));

  // Verify WAV file properties match our constants
  EXPECT_EQ(audio_file.getSampleRate(), 48000);  // kSampleRate
  EXPECT_EQ(audio_file.getNumChannels(), 2);     // kNumChannels
  EXPECT_EQ(audio_file.getBitDepth(), 32);       // 32-bit float

  // Should have some samples (at least a few blocks worth)
  EXPECT_GT(audio_file.getNumSamplesPerChannel(), 0);

  // Clean up
  std::filesystem::remove(test_file);
}

TEST_F(AudioRecordingTest, RecordingStopsAfterCodeUpdate) {
  const std::string test_file = "/tmp/soir_test_recording_stop.wav";

  // Remove test file if it exists
  std::filesystem::remove(test_file);

  // Start recording
  PushCode(R"(
sys.record("/tmp/soir_test_recording_stop.wav")
log("recording_started")
  )");

  EXPECT_TRUE(WaitForNotification("recording_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Update code without record() call - this should stop recording
  PushCode(R"(
log("code_updated_without_record")
  )");

  EXPECT_TRUE(WaitForNotification("code_updated_without_record"));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Verify file was created and saved when recording stopped
  EXPECT_TRUE(std::filesystem::exists(test_file));

  // Verify it's a valid WAV file
  AudioFile<float> audio_file;
  EXPECT_TRUE(audio_file.load(test_file));
  EXPECT_GT(audio_file.getNumSamplesPerChannel(), 0);

  // Store the original file size
  auto original_size = std::filesystem::file_size(test_file);

  // Wait a bit more to ensure no new data is being written
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // File size should not have changed (recording stopped)
  auto current_size = std::filesystem::file_size(test_file);
  EXPECT_EQ(original_size, current_size);

  // Clean up
  std::filesystem::remove(test_file);
}

TEST_F(AudioRecordingTest, RecordingContinuesWithSameFile) {
  const std::string test_file = "/tmp/soir_test_recording_continue.wav";

  // Remove test file if it exists
  std::filesystem::remove(test_file);

  // Start recording
  PushCode(R"(
sys.record("/tmp/soir_test_recording_continue.wav")
log("recording_started")
  )");

  EXPECT_TRUE(WaitForNotification("recording_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Continue recording with same file path
  PushCode(R"(
sys.record("/tmp/soir_test_recording_continue.wav")
log("recording_continued")
  )");

  EXPECT_TRUE(WaitForNotification("recording_continued"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Stop recording
  PushCode(R"(
log("recording_stopped")
  )");

  EXPECT_TRUE(WaitForNotification("recording_stopped"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Verify file exists and has accumulated samples from both recording periods
  EXPECT_TRUE(std::filesystem::exists(test_file));

  AudioFile<float> audio_file;
  EXPECT_TRUE(audio_file.load(test_file));
  EXPECT_GT(audio_file.getNumSamplesPerChannel(),
            1000);  // Should have substantial data

  // Clean up
  std::filesystem::remove(test_file);
}

TEST_F(AudioRecordingTest, RecordingWithDifferentFiles) {
  const std::string test_file1 = "/tmp/soir_test_recording_file1.wav";
  const std::string test_file2 = "/tmp/soir_test_recording_file2.wav";

  // Remove test files if they exist
  std::filesystem::remove(test_file1);
  std::filesystem::remove(test_file2);

  // Start recording to first file
  PushCode(R"(
sys.record("/tmp/soir_test_recording_file1.wav")
log("recording_file1_started")
  )");

  EXPECT_TRUE(WaitForNotification("recording_file1_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Switch to second file (should stop first and start second)
  PushCode(R"(
sys.record("/tmp/soir_test_recording_file2.wav")
log("recording_file2_started")
  )");

  EXPECT_TRUE(WaitForNotification("recording_file2_started"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Stop recording
  PushCode(R"(
log("recording_stopped")
  )");

  EXPECT_TRUE(WaitForNotification("recording_stopped"));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Both files should exist
  EXPECT_TRUE(std::filesystem::exists(test_file1));
  EXPECT_TRUE(std::filesystem::exists(test_file2));

  // Both should be valid WAV files
  AudioFile<float> audio_file1, audio_file2;
  EXPECT_TRUE(audio_file1.load(test_file1));
  EXPECT_TRUE(audio_file2.load(test_file2));

  EXPECT_GT(audio_file1.getNumSamplesPerChannel(), 0);
  EXPECT_GT(audio_file2.getNumSamplesPerChannel(), 0);

  // Clean up
  std::filesystem::remove(test_file1);
  std::filesystem::remove(test_file2);
}

}  // namespace test
}  // namespace core
}  // namespace soir
