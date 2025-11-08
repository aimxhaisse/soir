# Nocturne: RT and DSP Migration Plan

## Overview

This document outlines the migration strategy for moving the Real-Time (RT) and Digital Signal Processing (DSP) engines from the parent Soir project (`../src/core/`) into the Nocturne project. The migration follows an **incremental approach** where each step is independently compilable and testable.

## Architecture Goals

### Single Process, Multi-Thread Design
```
Python CLI Process
└─ Python Layer
    ├─ File Watcher Thread (watchdog)
    │   └─ Calls C++ to update code
    └─ C++ Engine (via pybind11)
        ├─ RT Thread (C++)
        │   ├─ Embedded Python Interpreter
        │   ├─ Temporal Scheduler
        │   └─ User Script Execution
        └─ Audio Thread (C++)
            ├─ miniaudio integration
            └─ Audio processing callbacks
```

### Key Design Principles
- **Incremental migration**: Each step must compile and pass tests
- **No TUI**: Simple CLI only, no terminal UI
- **No doc generation**: Remove mkdocs and related tooling
- **miniaudio over SDL**: Use miniaudio for cross-platform audio instead of SDL3
- **FetchContent dependencies**: Continue using CMake FetchContent, not vcpkg
- **Minimal Python deps**: Focus on core functionality (typer, flask, watchdog)

## Current State Analysis

### Parent Project Structure (`../src/core/`)
```
../src/core/
├── dsp/                    # 31 files, ~1926 lines - DSP algorithms
│   ├── biquad_filter.*
│   ├── low_pass_filter.*
│   ├── high_pass_filter.*
│   ├── band_pass_filter.*
│   ├── lpf.*
│   ├── delay.*
│   ├── modulated_delay.*
│   ├── comb_filter.*
│   ├── delayed_apf.*
│   ├── reverb.*
│   ├── chorus.*
│   ├── lfo.*
│   ├── *_shelving_filter.*
│   └── tools.*
├── rt/                     # 6 C++ files - Runtime engine
│   ├── bindings.*
│   ├── notifier.*
│   ├── runtime.*
│   └── py/soir/           # Python user API
├── fx/                    # 13 files - Effect wrappers
│   ├── fx_chorus.*
│   ├── fx_echo.*
│   ├── fx_hpf.*
│   ├── fx_lpf.*
│   ├── fx_reverb.*
│   ├── fx_stack.*
│   └── fx.hh
├── inst/                  # 5 files - Instruments
│   ├── instrument.hh
│   ├── midi_ext.*
│   └── sampler.*
└── [core files]           # Engine, buffers, parameters, etc.
    ├── engine.*
    ├── audio_buffer.*
    ├── audio_output.*
    ├── audio_recorder.*
    ├── sample.*
    ├── parameter.*
    ├── midi_stack.*
    ├── tools.*
    └── common.hh
```

### Nocturne Current Structure
```
nocturne/
├── cpp/
│   ├── bindings/          # pybind11 bindings
│   ├── src/
│   │   ├── core/          # soir.cc/hh only
│   │   └── utils/         # config, logger
│   └── tests/utils/       # config tests
├── python/
│   └── soir/              # Flask web app, CLI
└── CMakeLists.txt         # Uses FetchContent
```

## Incremental Migration Steps

Each step is designed to:
1. Add new code/dependencies
2. Build successfully
3. Pass all tests (including new ones)

---

## Step 1: Setup DSP Directory Structure

**Goal**: Create DSP directory and add minimal DSP files to establish build infrastructure.

### Actions
1. Create directory: `mkdir -p cpp/src/dsp cpp/tests/dsp`
2. Copy only DSP tools (foundation files):
   - `cp ../src/core/dsp/tools.hh cpp/src/dsp/`
   - `cp ../src/core/dsp/tools.cc cpp/src/dsp/`
3. Update include paths in `tools.cc/hh`:
   - `#include "core/dsp/tools.hh"` → `#include "dsp/tools.hh"`
4. Update CMakeLists.txt to add soir_dsp library:

```cmake
# Add after soir_utils library
add_library(soir_dsp
    cpp/src/dsp/tools.cc
)
target_include_directories(soir_dsp PUBLIC cpp/src)
target_link_libraries(soir_dsp
    absl::status
    absl::statusor
)
```

5. Create basic test `cpp/tests/dsp/tools_test.cc`:

```cpp
#include "dsp/tools.hh"
#include <gtest/gtest.h>

TEST(DspToolsTest, BasicTest) {
    // Add a simple test for any utility function in tools
    EXPECT_TRUE(true);
}
```

6. Add test to CMakeLists.txt:

```cmake
add_executable(dsp_tools_test cpp/tests/dsp/tools_test.cc)
target_link_libraries(dsp_tools_test soir_dsp gtest gtest_main)
add_test(NAME DspToolsTest COMMAND dsp_tools_test)
```

### Verification
```bash
just build          # Must succeed
just test          # Must pass (including new dsp_tools_test)
```

**Commit**: "feat: add DSP infrastructure with tools module"

---

## Step 2: Migrate Core DSP Filters

**Goal**: Add fundamental filter implementations.

### Actions
1. Copy biquad filter (base class):
   - `cp ../src/core/dsp/biquad_filter.{hh,cc} cpp/src/dsp/`
2. Copy basic filters:
   - `cp ../src/core/dsp/low_pass_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/high_pass_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/band_pass_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/lpf.{hh,cc} cpp/src/dsp/`
3. Update all include paths: `#include "core/dsp/..."` → `#include "dsp/..."`
4. Update CMakeLists.txt soir_dsp library:

```cmake
add_library(soir_dsp
    cpp/src/dsp/band_pass_filter.cc
    cpp/src/dsp/biquad_filter.cc
    cpp/src/dsp/high_pass_filter.cc
    cpp/src/dsp/low_pass_filter.cc
    cpp/src/dsp/lpf.cc
    cpp/src/dsp/tools.cc
)
```

5. Add filter tests `cpp/tests/dsp/filters_test.cc`:

```cpp
#include "dsp/biquad_filter.hh"
#include "dsp/low_pass_filter.hh"
#include <gtest/gtest.h>

TEST(FiltersTest, BiquadFilterCreation) {
    // Basic instantiation test
    EXPECT_TRUE(true);
}
```

6. Update test in CMakeLists.txt:

```cmake
add_executable(dsp_test
    cpp/tests/dsp/tools_test.cc
    cpp/tests/dsp/filters_test.cc
)
target_link_libraries(dsp_test soir_dsp gtest gtest_main)
add_test(NAME DspTest COMMAND dsp_test)
```

### Verification
```bash
just build
just test
```

**Commit**: "feat: add core DSP filter implementations"

---

## Step 3: Migrate Shelving Filters

**Goal**: Add shelving filters for EQ.

### Actions
1. Copy shelving filters:
   - `cp ../src/core/dsp/low_shelving_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/high_shelving_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/two_band_shelving_filter.{hh,cc} cpp/src/dsp/`
2. Update include paths
3. Update CMakeLists.txt:

```cmake
add_library(soir_dsp
    cpp/src/dsp/band_pass_filter.cc
    cpp/src/dsp/biquad_filter.cc
    cpp/src/dsp/high_pass_filter.cc
    cpp/src/dsp/high_shelving_filter.cc
    cpp/src/dsp/low_pass_filter.cc
    cpp/src/dsp/low_shelving_filter.cc
    cpp/src/dsp/lpf.cc
    cpp/src/dsp/tools.cc
    cpp/src/dsp/two_band_shelving_filter.cc
)
```

### Verification
```bash
just build
just test
```

**Commit**: "feat: add shelving filter implementations"

---

## Step 4: Migrate Delay Components

**Goal**: Add delay and related components.

### Actions
1. Copy delay files:
   - `cp ../src/core/dsp/delay.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/modulated_delay.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/comb_filter.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/delayed_apf.{hh,cc} cpp/src/dsp/`
2. Update include paths
3. Update CMakeLists.txt (add all new .cc files)
4. Add delay tests

### Verification
```bash
just build
just test
```

**Commit**: "feat: add delay and comb filter implementations"

---

## Step 5: Migrate LFO and Effects

**Goal**: Add LFO and complex effects (reverb, chorus).

### Actions
1. Copy LFO:
   - `cp ../src/core/dsp/lfo.{hh,cc} cpp/src/dsp/`
2. Copy effects:
   - `cp ../src/core/dsp/reverb.{hh,cc} cpp/src/dsp/`
   - `cp ../src/core/dsp/chorus.{hh,cc} cpp/src/dsp/`
3. Update include paths
4. Update CMakeLists.txt (all DSP files now included)
5. Add LFO and effects tests

### Verification
```bash
just build
just test
```

**Commit**: "feat: complete DSP library with LFO and effects"

---

## Step 6: Add Core Infrastructure (audio_buffer, common.hh)

**Goal**: Migrate core audio infrastructure without dependencies.

### Actions
1. Create audio directory: `mkdir -p cpp/src/audio cpp/tests/audio`
2. Copy files:
   - `cp ../src/core/audio_buffer.{hh,cc} cpp/src/audio/`
   - `cp ../src/core/common.hh cpp/src/core/`
3. Update include paths in audio_buffer files
4. Create soir_audio library in CMakeLists.txt:

```cmake
add_library(soir_audio
    cpp/src/audio/audio_buffer.cc
)
target_include_directories(soir_audio PUBLIC cpp/src)
target_link_libraries(soir_audio
    absl::status
    absl::statusor
)
```

5. Add audio buffer tests
6. Update _core module to link against soir_audio (if needed)

### Verification
```bash
just build
just test
```

**Commit**: "feat: add audio buffer infrastructure"

---

## Step 7: Add Sample, Parameter, and Tools to Core

**Goal**: Add core utilities that don't depend on audio I/O.

### Actions
1. Copy files:
   - `cp ../src/core/sample.{hh,cc} cpp/src/core/`
   - `cp ../src/core/parameter.{hh,cc} cpp/src/core/`
   - `cp ../src/core/tools.{hh,cc} cpp/src/core/`
2. Update include paths
3. Create soir_core_utils library or extend existing:

```cmake
add_library(soir_core_utils
    cpp/src/core/parameter.cc
    cpp/src/core/sample.cc
    cpp/src/core/tools.cc
)
target_include_directories(soir_core_utils PUBLIC cpp/src)
target_link_libraries(soir_core_utils
    soir_audio
    absl::status
    absl::statusor
)
```

4. Add tests for parameter and sample classes

### Verification
```bash
just build
just test
```

**Commit**: "feat: add core parameter and sample management"

---

## Step 8: Add miniaudio Dependency and Wrapper

**Goal**: Add miniaudio and create audio output wrapper.

### Actions
1. Add miniaudio to CMakeLists.txt FetchContent:

```cmake
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23
)
FetchContent_MakeAvailable(miniaudio)
```

2. Create `cpp/src/audio/audio_output.hh`:

```cpp
#pragma once

#include "absl/status/status.h"
#include <functional>

namespace soir::audio {

class AudioOutput {
public:
    using AudioCallback = std::function<void(float* output, int frame_count)>;

    absl::Status Init(int sample_rate, int channels, int buffer_size);
    absl::Status Start();
    absl::Status Stop();
    void SetCallback(AudioCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace soir::audio
```

3. Create minimal implementation in `cpp/src/audio/audio_output.cc` using miniaudio
4. Update soir_audio library to include audio_output.cc
5. Link against miniaudio
6. Write integration test that initializes audio (no actual playback needed)

### Verification
```bash
just build
just test
```

**Commit**: "feat: add miniaudio audio output wrapper"

---

## Step 9: Migrate Effects Layer (fx/)

**Goal**: Add effect wrappers that use DSP components.

### Actions
1. Create fx directory: `mkdir -p cpp/src/fx cpp/tests/fx`
2. Copy effects:
   - `cp ../src/core/fx/fx.hh cpp/src/fx/`
   - `cp ../src/core/fx/fx_stack.{hh,cc} cpp/src/fx/`
   - `cp ../src/core/fx/fx_chorus.{hh,cc} cpp/src/fx/`
   - `cp ../src/core/fx/fx_echo.{hh,cc} cpp/src/fx/`
   - `cp ../src/core/fx/fx_hpf.{hh,cc} cpp/src/fx/`
   - `cp ../src/core/fx/fx_lpf.{hh,cc} cpp/src/fx/`
   - `cp ../src/core/fx/fx_reverb.{hh,cc} cpp/src/fx/`
3. Update include paths (change DSP includes from `core/dsp/` to `dsp/`)
4. Create soir_fx library:

```cmake
add_library(soir_fx
    cpp/src/fx/fx_chorus.cc
    cpp/src/fx/fx_echo.cc
    cpp/src/fx/fx_hpf.cc
    cpp/src/fx/fx_lpf.cc
    cpp/src/fx/fx_reverb.cc
    cpp/src/fx/fx_stack.cc
)
target_include_directories(soir_fx PUBLIC cpp/src)
target_link_libraries(soir_fx
    soir_dsp
    soir_audio
    absl::status
)
```

5. Add FX tests

### Verification
```bash
just build
just test
```

**Commit**: "feat: add effects layer with DSP integration"

---

## Step 10: Add AudioFile and libremidi Dependencies

**Goal**: Add dependencies needed for instruments.

### Actions
1. Add to CMakeLists.txt FetchContent:

```cmake
FetchContent_Declare(
    audiofile
    GIT_REPOSITORY https://github.com/adamstark/AudioFile.git
    GIT_TAG 1.1.4
)

FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/jcelerier/libremidi.git
    GIT_TAG v5.3.1
)

FetchContent_MakeAvailable(audiofile libremidi)
```

2. Test that dependencies build correctly

### Verification
```bash
just build
just test
```

**Commit**: "feat: add audiofile and libremidi dependencies"

---

## Step 11: Migrate Instruments Layer

**Goal**: Add sampler and MIDI instruments.

### Actions
1. Create inst directory: `mkdir -p cpp/src/inst cpp/tests/inst`
2. Copy instrument files:
   - `cp ../src/core/inst/instrument.hh cpp/src/inst/`
   - `cp ../src/core/inst/sampler.{hh,cc} cpp/src/inst/`
   - `cp ../src/core/inst/midi_ext.{hh,cc} cpp/src/inst/`
3. Update include paths
4. Create soir_inst library:

```cmake
add_library(soir_inst
    cpp/src/inst/midi_ext.cc
    cpp/src/inst/sampler.cc
)
target_include_directories(soir_inst PUBLIC cpp/src)
target_link_libraries(soir_inst
    soir_audio
    soir_fx
    audiofile
    libremidi
    absl::status
)
```

5. Add instrument tests

### Verification
```bash
just build
just test
```

**Commit**: "feat: add instruments layer with sampler and MIDI support"

---

## Step 12: Migrate Engine and MIDI Stack

**Goal**: Add main audio engine without runtime dependencies.

### Actions
1. Copy engine files:
   - `cp ../src/core/engine.{hh,cc} cpp/src/core/`
   - `cp ../src/core/midi_stack.{hh,cc} cpp/src/core/`
2. Update include paths
3. Replace SDL audio calls with miniaudio calls in engine.cc
4. Create soir_engine library:

```cmake
add_library(soir_engine
    cpp/src/core/engine.cc
    cpp/src/core/midi_stack.cc
)
target_include_directories(soir_engine PUBLIC cpp/src)
target_link_libraries(soir_engine
    soir_audio
    soir_core_utils
    soir_fx
    soir_inst
    absl::status
    absl::time
)
```

5. Add engine tests (initialization, basic operations)

### Verification
```bash
just build
just test
```

**Commit**: "feat: add audio engine with miniaudio integration"

---

## Step 13: Migrate Runtime Layer (RT)

**Goal**: Add runtime with embedded Python interpreter.

### Actions
1. Create rt directory: `mkdir -p cpp/src/rt cpp/tests/rt`
2. Copy runtime core files (not bindings):
   - `cp ../src/core/rt/notifier.{hh,cc} cpp/src/rt/`
   - `cp ../src/core/rt/runtime.{hh,cc} cpp/src/rt/`
3. Update include paths in runtime files
4. Create soir_rt library:

```cmake
add_library(soir_rt
    cpp/src/rt/notifier.cc
    cpp/src/rt/runtime.cc
)
target_include_directories(soir_rt PUBLIC cpp/src)
target_link_libraries(soir_rt
    soir_engine
    pybind11::embed
    absl::status
    absl::synchronization
)
```

5. Add runtime tests (notifier, basic scheduling)

### Verification
```bash
just build
just test
```

**Commit**: "feat: add runtime layer with Python embedding"

---

## Step 14: Integrate Runtime into Main Bindings

**Goal**: Expose runtime to Python via pybind11 module with dedicated binding files per component.

### Actions
1. Analyze `../src/core/rt/bindings.{hh,cc}` to understand what needs to be exposed
2. Create dedicated binding files in `cpp/bindings/`:
   - `cpp/bindings/runtime.cc` - Runtime/scheduler bindings
   - `cpp/bindings/notifier.cc` - Notifier/event bindings
   - `cpp/bindings/engine.cc` - Engine bindings (if needed)
   - Each file should have a function like `void bind_runtime(py::module& m)`
3. Update `cpp/bindings/bindings.cc` to call all binding functions:

```cpp
// In bindings.cc
void bind_logger(py::module& m);
void bind_soir(py::module& m);
void bind_runtime(py::module& m);
void bind_notifier(py::module& m);
void bind_engine(py::module& m);

PYBIND11_MODULE(_core, m) {
    bind_logger(m);
    bind_soir(m);
    bind_runtime(m);
    bind_notifier(m);
    bind_engine(m);
}
```

4. Update `cpp/src/core/soir.cc` to use new engine and runtime
5. Update _core module in CMakeLists.txt:

```cmake
pybind11_add_module(_core
    cpp/bindings/bindings.cc
    cpp/bindings/engine.cc
    cpp/bindings/logger.cc
    cpp/bindings/notifier.cc
    cpp/bindings/runtime.cc
    cpp/bindings/soir.cc
    cpp/src/core/soir.cc
)
target_include_directories(_core PRIVATE cpp/src cpp)
target_link_libraries(_core PRIVATE
    soir_utils
    soir_engine
    soir_rt
    absl::status
    absl::statusor
    absl::log
    absl::log_initialize
)
```

6. Test Python can import and call basic functions:

```python
# python/tests/test_core_import.py
import soir._core as core

def test_import():
    assert core is not None
```

### Verification
```bash
just build
just test
uv run pytest python/tests/
```

**Commit**: "feat: integrate runtime into Python bindings"

---

## Step 15: Migrate Python User API

**Goal**: Add Python user-facing API from parent project.

### Actions
1. Create rt directory in Python: `mkdir -p python/soir/rt`
2. Copy Python modules:
   - `cp -r ../src/core/rt/py/soir/*.py python/soir/rt/`
   - Exclude `__pycache__`, `cli/` subdirectory
3. Update imports in all copied files:
   - Change any absolute imports to match new structure
   - Update imports of C++ bindings if needed
4. Update `python/soir/__init__.py` to export rt modules
5. Add Python tests for key modules (tracks, fx, sampler)

### Verification
```bash
just build
just test
uv run pytest python/tests/
```

**Commit**: "feat: add Python user API from v1"

---

## Step 16: Update CLI for Live Coding

**Goal**: Wire up file watching and code updates.

### Actions
1. Update `python/soir/cli.py` to add live coding command:

```python
import typer
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import soir._core as core

app = typer.Typer()

class CodeWatcher(FileSystemEventHandler):
    def on_modified(self, event):
        if event.src_path.endswith('.py'):
            with open(event.src_path) as f:
                code = f.read()
            core.update_code(code)

@app.command()
def live(watch_dir: str = "."):
    """Start live coding session"""
    core.init()
    core.start()

    observer = Observer()
    observer.schedule(CodeWatcher(), watch_dir, recursive=True)
    observer.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
        core.stop()
```

2. Test manual invocation

### Verification
```bash
just build
uv run soir live --help
```

**Commit**: "feat: add live coding CLI command with file watching"

---

## Step 17: Cleanup Python Dependencies

**Goal**: Remove unused documentation and TUI dependencies.

### Actions
1. Edit `pyproject.toml`:
   - Remove: `pdoc`, `markdown`, `markdown-it-py`, `pygments`, `types-markdown`
   - Keep: `typer`, `flask`, `watchdog`, `pytest`, `black`, `ruff`, `mypy`
2. Run `uv lock` to update lockfile
3. Run `uv sync` to verify dependencies install correctly

### Verification
```bash
uv sync
just test
```

**Commit**: "chore: remove doc generation dependencies"

---

## Step 18: Update justfile Commands

**Goal**: Add convenience commands for new workflow.

### Actions
1. Update `justfile` with new commands if needed:

```makefile
# Example additions
live DIR=".":
    uv run soir live {{DIR}}

check-all:
    just check
    just build
    just test
```

### Verification
```bash
just check-all
```

**Commit**: "chore: update justfile with new commands"

---

## Step 19: Write Integration Tests

**Goal**: Verify end-to-end functionality.

### Actions
1. Create integration test that:
   - Initializes engine
   - Starts audio
   - Executes simple Python code via runtime
   - Stops cleanly
2. Add to test suite

### Verification
```bash
just test
```

**Commit**: "test: add integration tests for live coding workflow"

---

## Step 20: Documentation and Final Cleanup

**Goal**: Document the new architecture and migration.

### Actions
1. Update CLAUDE.md with new structure
2. Update README if it exists
3. Remove any obsolete files or comments
4. Verify all tests pass
5. Run code formatters

### Verification
```bash
just check
just test
```

**Commit**: "docs: update documentation for new architecture"

---

## Migration Checklist

Track progress through each step:

- [x] Step 1: Setup DSP directory structure
- [x] Step 2: Migrate core DSP filters
- [x] Step 3: Migrate shelving filters
- [x] Step 4: Migrate delay components (includes LFO)
- [x] Step 5: Migrate effects (reverb, chorus)
- [x] Step 6: Add core infrastructure
- [x] Step 7: Add sample and tools (parameter skipped, depends on controls)
- [x] Step 8: Add miniaudio wrapper
- [ ] Step 9: Migrate effects layer
- [ ] Step 10: Add audiofile and libremidi
- [ ] Step 11: Migrate instruments layer
- [ ] Step 12: Migrate engine and MIDI stack
- [ ] Step 13: Migrate runtime layer
- [ ] Step 14: Integrate runtime into bindings
- [ ] Step 15: Migrate Python user API
- [ ] Step 16: Update CLI for live coding
- [ ] Step 17: Cleanup Python dependencies
- [ ] Step 18: Update justfile
- [ ] Step 19: Integration tests
- [ ] Step 20: Documentation and cleanup

## Final Directory Structure

After all steps are complete:

```
nocturne/
├── cpp/
│   ├── bindings/
│   │   ├── bindings.cc
│   │   ├── logger.cc
│   │   └── soir.cc
│   ├── src/
│   │   ├── audio/
│   │   │   ├── audio_buffer.{hh,cc}
│   │   │   └── audio_output.{hh,cc}
│   │   ├── core/
│   │   │   ├── common.hh
│   │   │   ├── engine.{hh,cc}
│   │   │   ├── midi_stack.{hh,cc}
│   │   │   ├── parameter.{hh,cc}
│   │   │   ├── sample.{hh,cc}
│   │   │   ├── soir.{hh,cc}
│   │   │   └── tools.{hh,cc}
│   │   ├── dsp/
│   │   │   ├── band_pass_filter.{hh,cc}
│   │   │   ├── biquad_filter.{hh,cc}
│   │   │   ├── chorus.{hh,cc}
│   │   │   ├── comb_filter.{hh,cc}
│   │   │   ├── delay.{hh,cc}
│   │   │   ├── delayed_apf.{hh,cc}
│   │   │   ├── high_pass_filter.{hh,cc}
│   │   │   ├── high_shelving_filter.{hh,cc}
│   │   │   ├── lfo.{hh,cc}
│   │   │   ├── low_pass_filter.{hh,cc}
│   │   │   ├── low_shelving_filter.{hh,cc}
│   │   │   ├── lpf.{hh,cc}
│   │   │   ├── modulated_delay.{hh,cc}
│   │   │   ├── reverb.{hh,cc}
│   │   │   ├── tools.{hh,cc}
│   │   │   └── two_band_shelving_filter.{hh,cc}
│   │   ├── fx/
│   │   │   ├── fx.hh
│   │   │   ├── fx_chorus.{hh,cc}
│   │   │   ├── fx_echo.{hh,cc}
│   │   │   ├── fx_hpf.{hh,cc}
│   │   │   ├── fx_lpf.{hh,cc}
│   │   │   ├── fx_reverb.{hh,cc}
│   │   │   └── fx_stack.{hh,cc}
│   │   ├── inst/
│   │   │   ├── instrument.hh
│   │   │   ├── midi_ext.{hh,cc}
│   │   │   └── sampler.{hh,cc}
│   │   ├── rt/
│   │   │   ├── bindings.{hh,cc}
│   │   │   ├── notifier.{hh,cc}
│   │   │   └── runtime.{hh,cc}
│   │   └── utils/
│   │       ├── config.{hh,cc}
│   │       └── logger.{hh,cc}
│   └── tests/
│       ├── audio/
│       ├── dsp/
│       ├── fx/
│       ├── inst/
│       ├── rt/
│       └── utils/
├── python/
│   ├── soir/
│   │   ├── rt/              # Migrated Python API
│   │   ├── www/             # Flask app
│   │   └── cli.py
│   └── tests/
├── CMakeLists.txt
├── pyproject.toml
├── justfile
└── PLAN.md
```

## Notes

### Key Principles
- **Each step is independent**: Can stop at any step and have a working system
- **Tests guard progress**: Every step must pass tests before proceeding
- **Incremental commits**: Each step gets its own commit for easy rollback
- **Compile-clean**: No "work in progress" that doesn't compile

### Breaking Changes
- Audio backend (SDL → miniaudio) may have different latency characteristics
- Test audio performance at Step 12 when engine is integrated
- May need to tune buffer sizes

### Estimated Timeline
- Steps 1-5 (DSP): 1-2 days
- Steps 6-8 (Audio infra): 1 day
- Steps 9-11 (FX/Inst): 1-2 days
- Steps 12-14 (Engine/RT/Bindings): 2-3 days
- Steps 15-20 (Python/CLI/Cleanup): 2-3 days
- **Total: ~1-2 weeks** (with incremental verification at each step)
