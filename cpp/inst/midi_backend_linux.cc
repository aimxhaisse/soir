#include <sys/stat.h>
#include <unistd.h>

#include "inst/midi_backend.hh"

namespace soir {
namespace inst {

bool MidiBackendAvailable() {
  // Probe the ALSA sequencer device node directly. libremidi opens it via
  // snd_seq_open() when constructing its backend; if that fails the
  // backend is left with a null handle and later crashes, so we must not
  // use it in that case.
  return access("/dev/snd/seq", R_OK | W_OK) == 0;
}

}  // namespace inst
}  // namespace soir
