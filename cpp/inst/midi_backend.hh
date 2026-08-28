#pragma once

namespace soir {
namespace inst {

// Returns whether libremidi MIDI backend objects (midi_out, observer) can
// be safely constructed and used to enumerate or open MIDI ports on this
// system.
//
// On Linux the ALSA sequencer backend is selected. libremidi tolerates the
// ALSA client creation failing (e.g. when /dev/snd/seq is not accessible
// to the current user) but then dereferences the resulting null handle in
// the observer constructor and the midi_out destructor, segfaulting the
// process. Callers must check this function before constructing any
// libremidi backend object and degrade gracefully when it returns false.
bool MidiBackendAvailable();

}  // namespace inst
}  // namespace soir
