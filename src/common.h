#ifndef SOIR_COMMON_H
#define SOIR_COMMON_H

#include <string>
#include <vector>

namespace soir {

using MidiMnemo = std::string;
using MidiMessage = std::vector<unsigned char>;
using MidiMessages = std::vector<MidiMessage>;

} // namespace soir

#endif // SOIR_COMMON_H
