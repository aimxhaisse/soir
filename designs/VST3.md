# VST3 Plugin Support

## Overview

Soir supports loading and using VST3 plugins — both **audio effects** and **instruments**. Plugins are discovered at startup from standard system locations. Effects can be added to any track's FX chain, and instruments can be used as the sound source for a track.

## Architecture

```
┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│   Python API    │ ───▶ │  C++ FX Stack    │ ───▶ │   VST3 SDK      │
│   fx.mk_vst()   │      │     FxVst        │      │   VstPlugin     │
└─────────────────┘      └──────────────────┘      └─────────────────┘

┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│   Python API    │ ───▶ │  C++ Instrument  │ ───▶ │   VST3 SDK      │
│ tracks.mk_vst() │      │    InstVst       │      │   VstPlugin     │
└─────────────────┘      └──────────────────┘      └─────────────────┘
```

### Components

| Component | Location | Description |
|-----------|----------|-------------|
| `VstHost` | `cpp/vst/vst_host.*` | Singleton managing plugin lifecycle, scanning, loading |
| `VstPlugin` | `cpp/vst/vst_plugin.*` | Wrapper for individual VST3 plugin instances |
| `VstScanner` | `cpp/vst/vst_scanner.*` | Discovers plugins from system directories |
| `FxVst` | `cpp/fx/fx_vst.*` | FX stack integration, parameter automation |
| `InstVst` | `cpp/inst/inst_vst.*` | VSTi instrument integration |
| Python API | `py/soir/rt/vst.py`, `py/soir/rt/fx.py`, `py/soir/rt/tracks.py` | User-facing API |

## Plugin Discovery

Plugins are scanned from standard locations at engine startup:

**macOS:**
- `/Library/Audio/Plug-Ins/VST3`
- `~/Library/Audio/Plug-Ins/VST3`

**Windows:**
- `C:\Program Files\Common Files\VST3`
- `C:\Program Files (x86)\Common Files\VST3`

**Linux:**
- `/usr/lib/vst3`
- `/usr/local/lib/vst3`
- `~/.vst3`

Plugins are classified as either `VstPluginType::kVstFx` or `VstPluginType::kVstInstrument` based on their VST3 subcategories.

## Usage

### List Available Plugins

```python
from soir.rt import vst

# All plugins
for plugin in vst.plugins():
    print(f"{plugin['name']} by {plugin['vendor']} ({plugin['type']})")

# Only instruments
for plugin in vst.instruments():
    print(f"{plugin['name']} by {plugin['vendor']}")

# Only effects
for plugin in vst.effects():
    print(f"{plugin['name']} by {plugin['vendor']}")
```

### Add VST Instrument Track

```python
from soir.rt import tracks, midi

tracks.setup({
    'synth': tracks.mk_vst('Diva', params={
        'cutoff': ctrl('filter_ctl'),
    }),
})

midi.note_on(60, track='synth')
```

### Add VST Effect to Track

```python
from soir.rt import tracks, fx

tracks.setup({
    'synth': tracks.mk_sampler(fxs={
        'eq': fx.mk_vst('FabFilter Pro-Q 3'),
        'reverb': fx.mk_vst('ValhallaShimmer', params={
            'mix': 0.5,
            'decay': ctrl('decay_ctrl'),
        }),
    }),
})
```

### Parameter Automation

Parameters can be automated using Soir's control system:

```python
ctrls.mk_lfo('wobble', rate=0.5)

tracks.setup({
    'lead': tracks.mk_sampler(fxs={
        'filter': fx.mk_vst('Plugin Name', params={
            'cutoff': ctrl('wobble'),  # LFO-controlled
            'resonance': 0.7,          # Static value
        }),
    }),
})
```

## Implementation Details

### Plugin Type Classification

```cpp
enum class VstPluginType { kVstFx, kVstInstrument };
```

Detected from VST3 subcategories during scanning. Instruments have `"Instrument"` in their subcategories and typically have 0 audio inputs.

### Thread Safety

All VST operations are mutex-protected:
- `VstHost`: Protected during plugin loading/scanning
- `VstPlugin`: Protected during process/parameter changes
- `FxVst`: Protected during render and updates
- `InstVst`: Protected during render and updates

### Audio Processing Flow

**Effects:**
1. `FxVst::Render()` called each audio block with MIDI events
2. Automated parameters updated from controls at current tick
3. `VstPlugin::Process()` called with audio buffer and events
4. Plugin processes audio in-place

**Instruments:**
1. `InstVst::Render()` called each audio block with MIDI events
2. Automated parameters updated from controls at current tick
3. MIDI events converted to VST3 `Event` structs with sample-accurate timing
4. `VstPlugin::Process()` generates audio into the buffer

### MIDI Event Delivery

MIDI events flow through the entire chain: instrument → FX stack. VST3's `IEventList` interface delivers events with sample-accurate timing via `Event::sampleOffset`. The `VstPlugin::Process(buffer, events)` signature is unified — FX plugins simply ignore events they don't need, while MIDI-driven FX (vocoders, gates) can consume them.

### Fast Updates

When track settings change:
- If plugin UID unchanged: `FastUpdate()` reloads only parameters
- If plugin UID changed: Full reinitialization required

### Plugin Info Structure

```cpp
enum class VstPluginType { kVstFx, kVstInstrument };

struct PluginInfo {
  std::string uid;              // VST3 component ID
  std::string name;             // Display name
  std::string vendor;           // Manufacturer
  std::string category;         // VST3 category (e.g., "Fx|EQ")
  std::string path;             // Bundle path
  int num_audio_inputs;         // Input channels (0 for instruments)
  int num_audio_outputs;        // Output channels (stereo = 2)
  VstPluginType type;           // Effect or instrument
};
```

## TUI Commands

```
vst list                    List available and instantiated VST plugins
vst open <track>            Open VST instrument editor
vst close <track>           Close VST instrument editor
vst open-fx <track>/<fx>    Open VST FX editor
vst close-fx <track>/<fx>   Close VST FX editor
```

## Dependencies

- **VST3 SDK** (v3.8.0): Fetched via CMake from Steinberg's GitHub
- Linked libraries: `sdk`, `sdk_hosting`
- macOS frameworks: `Cocoa`, `CoreFoundation`

## Limitations

- **Stereo only**: Currently hardcoded to 2-channel I/O
- **No state persistence**: Plugin presets not saved/loaded yet
- **macOS editor only**: GUI support implemented for macOS (Cocoa)

## File Structure

```
cpp/vst/
├── vst_host.cc/.hh          # Host singleton, plugin management
├── vst_plugin.cc/.hh        # Plugin wrapper, processing, IEventList
├── vst_scanner.cc/.hh       # Plugin discovery and classification
└── vst_editor_macos.mm      # macOS GUI window support

cpp/fx/
├── fx_vst.cc/.hh            # FX stack integration
└── fx_stack.cc              # VST case in FX factory

cpp/inst/
└── inst_vst.cc/.hh          # VST instrument integration

py/soir/rt/
├── vst.py                   # Python vst module (plugins, instruments, effects)
├── fx.py                    # mk_vst() for FX
└── tracks.py                # mk_vst() for instruments
```
