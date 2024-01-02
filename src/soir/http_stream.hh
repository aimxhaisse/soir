#pragma once

#include <absl/status/status.h>
#include <httplib.h>

#include "common.hh"

namespace maethstro {
namespace soir {

// Handles a long-lived HTTP stream, transcoding on the fly
// raw samples into the desired format.
class HttpStream : public SampleConsumer {
 public:
  HttpStream();
  virtual ~HttpStream();

  absl::Status PushSamples(const Samples& data) override;
  absl::Status Run(httplib::Response& response);
  absl::Status Stop();

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::list<Samples> stream_;
  bool running_;
};

}  // namespace soir
}  // namespace maethstro
