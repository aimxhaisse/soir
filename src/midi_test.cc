#include <queue>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "midi.h"

namespace soir {
namespace {

// DummyMidiSocket mocks a MidiSocket and provides facilities to
// simulate disconnections.
class DummyMidiSocket : public MidiSocket {
public:
  DummyMidiSocket() : disconnected_(false) {}
  void SetDisconnected(bool disconnected) { disconnected_ = disconnected; }
  void SetMessages(const MidiMessages &messages) {
    for (const auto &message : messages) {
      messages_.push(message);
    }
  }

  Status OpenPort(int port) {
    if (disconnected_) {
      return StatusCode::INTERNAL_MIDI_ERROR;
    }
    return StatusCode::OK;
  }

  Status GetMessage(MidiMessage *message) {
    if (disconnected_) {
      return StatusCode::INTERNAL_MIDI_ERROR;
    }

    if (!messages_.empty()) {
      *message = messages_.front();
      messages_.pop();
    }

    return StatusCode::OK;
  }

private:
  bool disconnected_;
  std::queue<MidiMessage> messages_;
};

class MidiRouterTest : public testing::Test {
public:
  void SetUp() { router_ = std::make_unique<MidiRouter>(); }

  void TearDown() { router_.reset(); }

  // Creates a MidiDevice and provide access to its internal socket.
  std::unique_ptr<MidiDevice> MakeDummyDevice(const std::string &name,
                                              DummyMidiSocket **socket) {
    std::unique_ptr<DummyMidiSocket> dummy_socket =
        std::make_unique<DummyMidiSocket>();
    *socket = dummy_socket.get();
    return std::make_unique<MidiDevice>(name, std::move(dummy_socket));
  }

  // Similar without providing access to the internal socket.
  std::unique_ptr<MidiDevice> MakeDummyDevice(const std::string &name) {
    DummyMidiSocket *socket;
    return MakeDummyDevice(name, &socket);
  }

protected:
  std::unique_ptr<MidiRouter> router_;
};

TEST_F(MidiRouterTest, RemoveOk) {
  EXPECT_EQ(router_->ProcessEvents(), StatusCode::OK);
}

TEST_F(MidiRouterTest, SyncDevices) {
  EXPECT_EQ(router_->SyncDevices(), StatusCode::OK);
}

} // namespace
} // namespace soir
