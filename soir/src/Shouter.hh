#pragma once

#include <JuceHeader.h>
#include <shout/shout.h>

#include "Config.hh"
#include "Status.hh"

namespace maethstro {

// C++ wrapper around C library shout.
struct Shouter {
  Status Init(const Config& config);
  Status Send(const void* data, std::size_t size);
  void Close();

 private:
  shout_t* shouter_;
};

}  // namespace maethstro
