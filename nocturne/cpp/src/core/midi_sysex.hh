#pragma once

#include <cstdint>
#include <string>

namespace soir {

enum class MidiSysexType : uint8_t {
  UNKNOWN = 0,
  UPDATE_CONTROLS = 1,
  SAMPLER_PLAY = 2,
  SAMPLER_STOP = 3,
};

struct MidiSysexInstruction {
  MidiSysexType type = MidiSysexType::UNKNOWN;
  std::string json_payload;

  bool ParseFromBytes(const uint8_t* data, size_t size);
  std::string SerializeToBytes() const;
};

}  // namespace soir
