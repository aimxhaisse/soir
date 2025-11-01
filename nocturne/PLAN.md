# Soir v2 Live Coding Migration Plan

## Proposed v2 Architecture

### Single Process, Multi-Thread Design
```
Python CLI Process
└─ Python
    ├─ File Watcher Thread (Python)
    │   └─ Calls C++ to update code of the RT thread
└─ C++ Engine (via pybind11)
    ├─ RT Thread (C++)
    │   ├─ Embedded Python Interpreter
    │   ├─ Temporal Scheduler
    │   └─ User Script Execution
    └─ DSP/Audio Thread (C++)
        ├─ miniaudio integration
        └─ Audio processing callbacks
```

### Communication Flow
```
File change
    ↓
File Watcher Thread detects change in code files
    ↓
Calls : soir.code_update(code)
    ↓
RT Thread queues code (same as v1 pattern)
    ↓
RT Thread executes py::exec() in embedded interpreter
    ↓
User script schedules audio via direct calls to DSP Thread
```

## Implementation Plan

### Core Engine Structure

**C++ Engine Class (pybind11 module)**:
```cpp
class Soir {
public:
    // Called from Python CLI
    absl::Status Init(conast std::string config_path);
    absl::Status Start();

    absl::Status Stop();

    void UpdateCode(const std::string& code);

private:
    std::unique_ptr<Engine> dsp_;
    std::unique_ptr<rt::Runtime> rt_;
};
```

**Python CLI (minimal)**:
```python
import typer
import soir._core as engine

@typer.command()
def start(watch_dir: str = "."):
    """Start Soir live coding engine"""
    engine.start()
```

### Code Watcher

Use Python file watcher.

## Migration Strategy

### Dependencies to Remove
- ✅ **efsw** → C++ std::filesystem
- ✅ **gRPC + Protobuf** → Direct function calls
- ✅ **SDL3** → miniaudio
- ✅ **Agent/Core separation** → Single process

### Dependencies to Keep
- ✅ **pybind11** (for Python ↔ C++ binding)
- ✅ **absl** (logging, status, time utilities)
- ✅ **Existing soir Python modules** (user API compatibility)

### Code Reuse from v1
- ✅ **Runtime class logic** (temporal scheduling, Python execution)
- ✅ **Python soir modules** (tracks, fx, sampler, etc.)
- ✅ **DSP algorithms** (effects, synthesis, etc.)
- ✅ **Configuration system**
