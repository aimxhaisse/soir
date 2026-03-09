#include <pybind11/pybind11.h>

#include "audio/pcm_stream.hh"
#include "bindings/bind.hh"
#include "bindings/rt.hh"
#include "core/engine.hh"

namespace py = pybind11;

namespace soir {
namespace bindings {

void Bind::PyPcm(py::module_& m) {
  auto pcm = m.def_submodule("pcm", "PCM audio stream for WebSocket streaming");

  pcm.def("get_current_offset_", []() -> size_t {
    auto* dsp = soir::rt::bindings::GetDsp();
    if (!dsp) {
      return 0;
    }
    return dsp->GetPcmStream()->GetCurrentOffset();
  });

  // Blocks until the engine produces a new block (condition variable, ~10ms
  // natural cadence at kBlockSize/kSampleRate). Returns (bytes, new_offset).
  // The bytes are interleaved float32 stereo PCM, ready for Float32Array in JS.
  pcm.def(
      "read_",
      [](size_t offset) -> py::tuple {
        auto* dsp = soir::rt::bindings::GetDsp();
        if (!dsp) {
          return py::make_tuple(py::bytes("", 0), offset);
        }
        auto result = dsp->GetPcmStream()->Read(offset);
        auto bytes =
            py::bytes(reinterpret_cast<const char*>(result.data.data()),
                      result.data.size() * sizeof(float));
        return py::make_tuple(bytes, result.new_offset);
      },
      py::arg("offset"));
}

}  // namespace bindings
}  // namespace soir
