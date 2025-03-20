#include "utils/misc.hh"

#include "core/engine/adsr.hh"
#include "core/engine/dsp.hh"

namespace soir {
namespace engine {

absl::Status ADSR::Init(float a, float d, float r, float level) {
  if (a < 0.0) {
    return absl::InvalidArgumentError("Attack must be > 0");
  }
  if (d < 0.0) {
    return absl::InvalidArgumentError("Decay must be > 0");
  }
  if (r < 0.0) {
    return absl::InvalidArgumentError("Release must be > 0");
  }
  if (level < 0.0 || level > 1.0) {
    return absl::InvalidArgumentError("Sustain level not in [0,1]");
  }

  attackMs_ = a;
  decayMs_ = d;
  sustainLevel_ = level;
  releaseMs_ = r;

  if (attackMs_) {
    // Attack will move the enveloppe linearly between 0.0f to 1.0f
    // from sample 0 to sample N. We compute here the increment of the
    // envelope;
    const float n = kSampleRate * (attackMs_ / 1000.0f);

    attackInc_ = 1.0f / n;
  }

  if (decayMs_) {
    // Decay kicks in the moment the attack phase completes, it starts
    // from 1.0 towards sustain level.
    const float n = kSampleRate * (decayMs_ / 1000.0f);

    decayDec_ = (1.0f - sustainLevel_) / n;
  }

  if (releaseMs_) {
    // Release will move the envelope linearly between sustain to 0.0f
    // whenever a note off event is triggered.
    const float n = kSampleRate * (releaseMs_ / 1000.0f);

    releaseDec_ = sustainLevel_ / n;
  }

  return absl::OkStatus();
}

void ADSR::Reset() {
  envelope_ = 0.0f;
  currentState_ = NONE;
}

void ADSR::NoteOn() {
  if (attackMs_ > 0.0f) {
    envelope_ = 0.0f;
    currentState_ = ATTACK;
    return;
  }

  if (decayMs_ >= 0.0f) {
    envelope_ = 1.0f;
    currentState_ = DECAY;
    return;
  }

  envelope_ = sustainLevel_;
  currentState_ = SUSTAIN;
}

void ADSR::NoteOff() {
  if (currentState_ == NONE) {
    return;
  }

  if (releaseMs_ >= 0.0) {
    currentState_ = RELEASE;
    return;
  }

  Reset();
}

float ADSR::GetNextEnvelope() {
  switch (currentState_) {
    case NONE:
      break;

    case ATTACK:
      envelope_ += attackInc_;

      if (envelope_ >= 1.0f) {
        envelope_ = 1.0f;
        currentState_ = decayMs_ > 0 ? DECAY : SUSTAIN;
      }
      break;

    case DECAY:
      envelope_ -= decayDec_;

      if (envelope_ <= sustainLevel_) {
        currentState_ = SUSTAIN;
        envelope_ = sustainLevel_;
      }
      break;

    case SUSTAIN:
      envelope_ = sustainLevel_;
      break;

    case RELEASE:
      envelope_ -= releaseDec_;

      if (envelope_ <= 0.0f) {
        envelope_ = 0.0f;
        currentState_ = NONE;
      }
      break;
  }

  return envelope_;
}

}  // namespace engine
}  // namespace soir
