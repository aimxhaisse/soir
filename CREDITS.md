# Sample pack credits

Provenance and licenses for the sample packs shipped in `lib/samples/`:
`std-bass`, `std-drums`, `std-fx`, `std-leads`, `std-loops`, `std-pads`.

License policy: CC0 / public domain preferred, CC-BY allowed with
per-sample attribution; CC-BY-SA and redistribution-restricted sources
excluded.

Except for the three sourced `std-drums` samples listed below, all
samples are **self-authored and CC0** (deterministic renders, fixed RNG
seeds). No external material is used and no attribution is required
for them.

## std-bass

| Pack sample | Description |
|---|---|
| `bass-808` | 808 sine kick-bass (sweep 110→55 Hz, rings down sub-octave, click + saturation) |
| `bass-808-long` | same, longer tail |
| `bass-sub` | clean A1 sine sub, fast decay |
| `bass-sub-fat` | A1 sine + 2nd harmonic + soft saturation |
| `bass-saw` | 2× detuned saws + sub, lowpassed |
| `bass-acid` | resonant-filter saw with fast brightness decay |
| `bass-pluck` | saw + pluck transient, fast filter-close |
| `bass-pwm` | square with slow duty-cycle wobble, lowpassed |
| `bass-square` | hollow square sub, gentle lowpass |
| `bass-finger` | additive electric-bass partials (fundamental-heavy) |
| `bass-pick` | additive electric-bass partials (brighter, tighter) |
| `bass-slap` | noise slap transient + pitch-drop fundamental |

## std-drums

Sourced samples:

| Pack sample | Original file (Wikimedia Commons) | Author | URL | License | Downloaded |
|---|---|---|---|---|---|
| `crash-1` | File:More cymbal sounds.ogg (segment 31.44–34.44 s) | aradlaw (transferred from pdsounds.org) | <https://commons.wikimedia.org/wiki/File:More_cymbal_sounds.ogg> | Public domain | 2026-08-19 |
| `crash-2` | File:More cymbal sounds.ogg (segment 0.80–2.11 s) | aradlaw (transferred from pdsounds.org) | <https://commons.wikimedia.org/wiki/File:More_cymbal_sounds.ogg> | Public domain | 2026-08-19 |
| `kick-punchy` | File:Kick (Gravity Sound).wav (hit #1 of 32-hit sequence) | Gravity Sound | <https://commons.wikimedia.org/wiki/File:Kick_(Gravity_Sound).wav> | CC BY 4.0 | 2026-08-19 |

Self-authored (CC0):

| Pack sample | Description |
|---|---|
| `kick-808` | 808 sine kick (pitch sweep 150→45 Hz, exp decay + click) |
| `snare-909` | 909-style snare (BP noise + 186 Hz sine) |
| `snare-crack` | Acoustic-style snare (tuned membrane + noise burst) |
| `tom-low` / `tom-mid` / `tom-high` | Tuned membrane toms (sine decay + noise attack) |
| `hat-closed` / `hat-open` | 909/808-style noise hats (HP ~7 kHz, short/long decay) |
| `clap-1` / `clap-2` | 909-style claps (stacked BP noise bursts) |

## std-fx

| Pack sample | Description |
|---|---|
| `fx-riser-1` | tight riser: HP noise + 200→2400 Hz sine sweep, 3 s |
| `fx-riser-2` | airy riser: BP crossfade up + speeding tremolo, 2 s |
| `fx-downer-1` | reverse swell: HP noise swell + hard cut + falling sine |
| `fx-downer-2` | filter sweep down (HP → LP crossfade), 2 s |
| `fx-impact-1` | big hit: click + 100→32 Hz sub drop + wide noise, 2.5 s |
| `fx-impact-2` | smaller/higher hit, 1.2 s |
| `fx-whoosh-1` | noise with up-then-down brightness arc, 1 s |
| `fx-whoosh-2` | falling whoosh (bright start, dark end), 0.7 s |
| `fx-sweep-1` | bright noise bed opening mid-phrase, 2 s |
| `fx-vinylstop` | detuned saws with falling-pitch time warp + crackle |
| `fx-boom` | slow 55→28 Hz sub drop, saturated, 1.8 s |
| `fx-glitch` | 30 Hz-gated 16-level quantized noise, 0.5 s |

## std-leads

| Pack sample | Description |
|---|---|
| `lead-saw` | 2× detuned saws, lowpassed, delay-on vibrato |
| `lead-supersaw` | 4× detuned saws, wide body |
| `lead-square` | hollow square + sub-octave square, lowpassed |
| `lead-pulse` | square with slow duty-cycle wobble, lowpassed |
| `lead-pluck` | saw with fast filter-close decay + pluck transient |
| `lead-stab` | bright stab hit (saw + 7th) with noise attack |
| `lead-fm` | 2-op FM (sine carrier, 3:1 modulator, index decay) |
| `lead-glass` | additive airy harmonics (1/2/3/5/7), staggered decays |
| `lead-bell` | inharmonic partials (1 : 2.01 : 2.99 : 4.2) + strike |
| `lead-marimba` | mallet: fundamental + woody partials, fast decay |
| `lead-organ` | additive drawbar-style partials with shimmer |
| `lead-vox` | detuned saws through 3 formant bandpasses |

## std-loops

| Pack sample | Hits used (all CC0, self-authored) | Source pack |
|---|---|---|
| `loop-house-124` | kick-808, clap-1, hat-closed, hat-open, bass-808 | std-drums, std-bass |
| `loop-techno-132` | kick-808, clap-1, hat-closed, hat-open, bass-acid | std-drums, std-bass |
| `loop-dnb-174` | kick-808, snare-909, hat-closed, bass-saw, bass-sub | std-drums, std-bass |
| `loop-break-90` | kick-808, snare-909, snare-crack, hat-closed, bass-808, tom-low/mid/high | std-drums, std-bass |
| `loop-ambient-112` | pad-warm, pad-airy, snare-crack, hat-closed | std-pads, std-drums |

Loops use only CC0 hits — the CC-BY `std-drums` picks (`crash-1`,
`crash-2`, `kick-punchy`) are deliberately excluded so no attribution
obligations attach to the rendered patterns.

## std-pads

| Pack sample | Description |
|---|---|
| `pad-warm` | additive low harmonics, slow attack, long release |
| `pad-analog` | 3× detuned saws through a slowly opening lowpass |
| `pad-choir` | detuned saws through 3 formant bandpasses + low body |
| `pad-airy` | high shimmer harmonics + soft bandpassed noise bed |
| `pad-retro` | detuned saws with 2-tap short chorus |
| `pad-film` | sub-octave swell + evolving lowpassed noise |
| `pad-space` | shimmer harmonics with slow filter LFO drift |
| `pad-dream` | vibrato saws + 3-tap feedback echo tail |
| `pad-tape` | filtered saws with wow & flutter rate wobble |
| `pad-pulse` | soft square with slow duty-cycle breathing |
| `pad-sine` | fundamental + sub-octave drone, longest release |
| `pad-sweep` | saws brightening mid-phrase, then receding |
