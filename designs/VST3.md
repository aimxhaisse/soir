# VST3 Plugin Support

## Overview

Soir supports loading and using VST3 audio effect plugins. Plugins are discovered at startup from standard system locations and can be added to any track's FX chain.

## Architecture

```
┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│   Python API    │ ───▶ │  C++ FX Stack    │ ───▶ │   VST3 SDK      │
│   fx.mk_vst()   │      │     FxVst        │      │   VstPlugin     │
└─────────────────┘      └──────────────────┘      └─────────────────┘
```

### Components

| Component | Location | Description |
|-----------|----------|-------------|
| `VstHost` | `cpp/vst/vst_host.*` | Singleton managing plugin lifecycle, scanning, loading |
| `VstPlugin` | `cpp/vst/vst_plugin.*` | Wrapper for individual VST3 plugin instances |
| `VstScanner` | `cpp/vst/vst_scanner.*` | Discovers plugins from system directories |
| `FxVst` | `cpp/fx/fx_stack.cc` | FX stack integration, parameter automation |
| Python API | `py/soir/rt/vst.py`, `py/soir/rt/fx.py` | User-facing API |

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

## Usage

### List Available Plugins

```python
from soir.rt import vst

for plugin in vst.plugins():
    print(f"{plugin['name']} by {plugin['vendor']}")
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

### Thread Safety

All VST operations are mutex-protected:
- `VstHost`: Protected during plugin loading/scanning
- `VstPlugin`: Protected during process/parameter changes
- `FxVst`: Protected during render and updates

### Audio Processing Flow

1. `FxVst::Render()` called each audio block
2. Automated parameters updated from controls at current tick
3. `VstPlugin::Process()` called with audio buffer
4. Plugin processes audio in-place

### Fast Updates

When track settings change:
- If plugin UID unchanged: `FastUpdate()` reloads only parameters
- If plugin UID changed: Full reinitialization required

### Plugin Info Structure

```cpp
struct PluginInfo {
  std::string uid;              // VST3 component ID
  std::string name;             // Display name
  std::string vendor;           // Manufacturer
  std::string category;         // VST3 category (e.g., "Fx|EQ")
  std::string path;             // Bundle path
  int num_audio_inputs;         // Input channels (stereo = 2)
  int num_audio_outputs;        // Output channels (stereo = 2)
};
```

## Dependencies

- **VST3 SDK** (v3.8.0): Fetched via CMake from Steinberg's GitHub
- Linked libraries: `sdk`, `sdk_hosting`
- macOS frameworks: `Cocoa`, `CoreFoundation`

## Limitations

- **Stereo only**: Currently hardcoded to 2-channel I/O
- **No state persistence**: Plugin presets not saved/loaded yet
- **macOS editor only**: GUI support implemented for macOS (Cocoa)
- **Effects only**: Instrument plugins (VSTi) not supported

## File Structure

```
cpp/vst/
├── vst_host.cc/.hh          # Host singleton, plugin management
├── vst_plugin.cc/.hh        # Plugin wrapper, processing
├── vst_scanner.cc/.hh       # Plugin discovery
└── vst_editor_macos.mm      # macOS GUI window support

cpp/fx/
├── fx_vst.cc/.hh            # FX stack integration
└── fx_stack.cc              # VST case in FX factory

py/soir/rt/
├── vst.py                   # Python vst module
└── fx.py                    # mk_vst() function
```
