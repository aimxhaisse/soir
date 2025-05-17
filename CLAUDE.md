# Soir Development Guide

## Build Commands
- Build: `just dev-build`
- Test all: `just dev-test`
- Run single test: `just dev-test "TestName*"` (e.g., `just dev-test "LFOTest*"`)
- Run Soir engine: `just dev-run`
- Generate docs: `just dev-mk-docs`

## Code Style Guidelines

### C++
- Follow Google C++ style guide (see .clang-format)
- Use snake_case for variables and functions
- Use CamelCase for class names
- Header files use .hh extension, implementation files use .cc
- Use absl::Status for error handling and propagation
- Use std::unique_ptr for ownership semantics
- Namespaces: soir::{component} (e.g., soir::dsp)

### Python
- Use snake_case for functions and variables
- Use CamelCase for classes
- Type hints required for function parameters and return values
- Use native type hints, never import things from the typing package
- Docstrings in Google style with Args/Returns/Raises sections
- 80 character line limit
- Import order: standard lib, then third-party, then project-specific
- Prefer explicit error handling with custom exceptions from soir.errors
