#pragma once

#include <absl/status/status.h>

namespace neon {
namespace dsp {

// ADSR envelope that can be used to:
//
// - NoteOn kicks in the envelope
// - NoteOff kicks in the release phase
//
// To avoid glitches, care must be taken to properly call NoteOff
// before the end of the audio buffer if it's not ending smoothly.
class ADSR {
 public:
  // Can be called multiple times while playing: it will only affect
  // the duration of the current phase without glitching (steps will
  // stretch in time without creating a too big jump for the
  // envelope).
  absl::Status Init(float attackMs, float decayMs, float sustainLevel,
                    float releaseMs);
  void Reset();
  void NoteOn();
  void NoteOff();
  float GetNextEnvelope();
  float GetSustainLevel() const { return sustainLevel_; }

 private:
  enum State { NONE, ATTACK, DECAY, SUSTAIN, RELEASE };

  // Configuration.
  float attackMs_ = 100.0;
  float decayMs_ = 1000.0;
  float sustainLevel_ = 1.0;
  float releaseMs_ = 100.0;

  float envelope_ = 0;
  float attackInc_ = 0.0f;
  float decayDec_ = 0.0f;
  float releaseDec_ = 0.0f;
  State currentState_ = NONE;
};

}  // namespace dsp
}  // namespace neon
