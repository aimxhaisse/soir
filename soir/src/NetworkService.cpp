#include "NetworkService.hh"

#include <glog/logging.h>
#include <math.h>

#include "Constants.hh"

namespace maethstro {

// Size of the MIDI buffer.
static constexpr int kBufferSize = 512;

StatusOr<std::unique_ptr<NetworkService>> NetworkService::InitFromConfig(
    const Config& config, IMidiConsumer* consumer) {
  LOG(INFO) << "initializing network service";

  auto service = std::make_unique<NetworkService>();

  service->midiConsumer_ = consumer;

  if (!service->socket_.createListener(config.port_, config.host_)) {
    RETURN_ERROR(StatusCode::INVALID_CONFIG, "unable to bind network service "
                                                 << "host=" << config.host_
                                                 << ", port=" << config.port_);
  }

  LOG(INFO) << "socket bound to host = " << config.host_
            << ", port = " << config.port_;

  return service;
}

void NetworkService::handleIncomingMidiMessage(void*, juce::MidiMessage msg) {
  LOG(INFO) << "got MIDI message " << msg.getDescription();

  // Not sure if it's correct, but it sounds correct. Struggled to get
  // it right in a tired mindset, it may be approached in a simpler
  // way.
  const double sInBlock = (static_cast<double>(kBlockSize) / kSampleRate);
  const double sampleInMs =
      fmod(juce::Time::getMillisecondCounterHiRes(), sInBlock * 1000.0);

  // Why clip it? Due to to precision conversions, sometimes it
  // actually is sligthly bigger than the block size, despite being
  // clipped by the fmod above to a block size.
  const int sample = std::max(
      kBlockSize - 1, static_cast<int>((sampleInMs * kSampleRate) / 1000.0));

  midiConsumer_->PushMessage(msg, sample);
}

void NetworkService::Stop() {
  LOG(INFO) << "asking network thread to stop";
  socket_.close();
}

Status NetworkService::Loop(std::atomic<bool>& stop) {
  while (true) {
    LOG(INFO) << "waiting for incoming connection...";
    client_.reset(socket_.waitForNextConnection());
    if (!client_) {
      stop = true;
      RETURN_ERROR(StatusCode::INTERNAL_ERROR,
                   "unable to get client connection");
    }
    LOG(INFO) << "connection accepted";

    // Here we reset the MIDI parser in case we have some data from the previous
    // connection, we don't want to mess with it anymore.
    midiParser_.reset(new juce::MidiDataConcatenator(kBufferSize));

    char buffer[kBufferSize];
    memset(buffer, 0, sizeof(buffer));

    while (true) {
      if (stop) {
        LOG(INFO) << "stopping connection with client";
        client_->close();
        break;
      }

      if (client_->waitUntilReady(true, 500)) {
        const int nbRead = client_->read(buffer, kBufferSize, false);

        if (nbRead) {
          midiParser_->pushMidiData<void*, NetworkService&>(buffer, nbRead, 0,
                                                            nullptr, *this);
        } else {
          client_->close();
        }
      }

      if (!client_->isConnected()) {
        LOG(INFO) << "connection closed";
        break;
      }
    }
  }

  return StatusCode::OK;
}

}  // namespace maethstro
