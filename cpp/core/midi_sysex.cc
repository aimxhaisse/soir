#include "core/midi_sysex.hh"

namespace soir {

bool MidiSysexInstruction::ParseFromBytes(const uint8_t* data, size_t size) {
  if (size < 1) {
    return false;
  }

  type = static_cast<MidiSysexType>(data[0]);

  if (size > 1) {
    json_payload =
        std::string(reinterpret_cast<const char*>(data + 1), size - 1);
  }

  return true;
}

std::string MidiSysexInstruction::SerializeToBytes() const {
  std::string result;
  result.push_back(static_cast<char>(type));
  result += json_payload;
  return result;
}

}  // namespace soir
