#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "audio/audio_buffer.hh"
#include "core/common.hh"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "vst/vst_host.hh"

namespace soir {

class MidiEventAt;  // Forward declaration to avoid heavy libremidi include.

namespace vst {

struct VstParameter {
  uint32_t id;
  std::string name;
  std::string short_name;
  float default_value;
  float min_value;
  float max_value;
  int step_count;
};

// Simple IEventList implementation for passing MIDI events to VST3 plugins.
class EventList : public Steinberg::Vst::IEventList {
 public:
  EventList() = default;

  Steinberg::int32 PLUGIN_API getEventCount() override;
  Steinberg::tresult PLUGIN_API getEvent(Steinberg::int32 index,
                                         Steinberg::Vst::Event& e) override;
  Steinberg::tresult PLUGIN_API addEvent(Steinberg::Vst::Event& e) override;

  void Clear();

  // FUnknown
  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,
                                               void** obj) override;
  Steinberg::uint32 PLUGIN_API addRef() override;
  Steinberg::uint32 PLUGIN_API release() override;

 private:
  std::vector<Steinberg::Vst::Event> events_;
  std::atomic<Steinberg::int32> ref_count_{1};
};

class VstPlugin {
 public:
  VstPlugin();
  ~VstPlugin();

  absl::Status Init(const std::string& path, Steinberg::FUnknown* host_context,
                    VstPluginType type);
  absl::Status Shutdown();
  absl::Status Activate(int sample_rate, int block_size);
  absl::Status Deactivate();

  void Process(AudioBuffer& buffer, const std::list<MidiEventAt>& events);

  VstPluginType GetType() const { return type_; }

  std::map<std::string, VstParameter> GetParameters();
  absl::Status SetParameter(uint32_t id, float value);
  absl::StatusOr<float> GetParameter(uint32_t id);

  bool HasEditor();
  absl::Status OpenEditor(void* parent_window);
  absl::Status CloseEditor();
  bool IsEditorOpen();
  std::pair<int, int> GetEditorSize() const;

  absl::StatusOr<std::vector<uint8_t>> SaveState();
  absl::Status LoadState(const std::vector<uint8_t>& state);

 private:
  void PopulateEventList(SampleTick block_start_tick,
                         const std::list<MidiEventAt>& events);

  std::mutex mutex_;
  VstPluginType type_ = VstPluginType::kVstFx;
  bool activated_ = false;
  bool editor_open_ = false;
  std::pair<int, int> editor_size_ = {800, 600};
  EventList input_events_;
  EventList output_events_;

  VST3::Hosting::Module::Ptr module_;
  Steinberg::IPtr<Steinberg::Vst::IComponent> component_;
  Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> processor_;
  Steinberg::IPtr<Steinberg::Vst::IEditController> controller_;
  Steinberg::FUnknownPtr<Steinberg::IPlugView> view_;

  Steinberg::Vst::ProcessSetup process_setup_;
  Steinberg::Vst::ProcessData process_data_;
  Steinberg::Vst::AudioBusBuffers input_buffers_;
  Steinberg::Vst::AudioBusBuffers output_buffers_;

  std::vector<float> input_left_;
  std::vector<float> input_right_;
  std::vector<float> output_left_;
  std::vector<float> output_right_;
  float* input_ptrs_[2];
  float* output_ptrs_[2];
};

}  // namespace vst
}  // namespace soir
