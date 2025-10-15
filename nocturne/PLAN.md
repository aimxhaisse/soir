# Soir v2 Live Coding Migration Plan

## Proposed v2 Architecture

### Single Process, Multi-Thread Design
```
Python CLI Process
└─ C++ Engine (via pybind11)
    ├─ File Watcher Thread (C++)
    │   ├─ std::filesystem monitoring
    │   └─ Direct calls to RT Thread
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
File Watcher Thread detects change
    ↓
Direct C++ function call: rt_thread.push_code_update(code)
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
class SoirEngine {
public:
    // Called from Python CLI
    absl::Status Init(const Config& config);
    absl::Status Start();

    absl::Status Stop();

    void UpdateCode(const std::string& code);

private:
    std::unique_ptr<CodeWatcher> code_watcher_;
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

**Replace efsw with C++ std::filesystem**:
```cpp
class CodeWatcher {
    void run(const std::string& directory) {
        while (running_) {
            for (auto& change : detect_changes()) {
                if (is_python_file(change.path)) {
                    auto code = read_file(change.path);
                    engine_->UpdateCode(code);
                }
            }
            std::this_thread::sleep_for(100ms);  // Or event-based
        }
    }
};
```

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
