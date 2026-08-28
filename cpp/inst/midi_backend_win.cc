#include "inst/midi_backend.hh"

namespace soir {
namespace inst {

// The MME backend has no null-handle failure mode like the Linux ALSA
// sequencer backend, so libremidi objects are always safe to construct.
bool MidiBackendAvailable() { return true; }

}  // namespace inst
}  // namespace soir
