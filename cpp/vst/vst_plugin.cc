#include "vst/vst_plugin.hh"

#include <absl/log/log.h>

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

namespace soir {
namespace vst {

using namespace Steinberg;
using namespace Steinberg::Vst;

VstPlugin::VstPlugin()
    : activated_(false),
      editor_open_(false),
      input_ptrs_{nullptr, nullptr},
      output_ptrs_{nullptr, nullptr} {}

VstPlugin::~VstPlugin() = default;

absl::Status VstPlugin::Shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (activated_) {
    if (component_) {
      component_->setActive(false);
    }
    activated_ = false;
  }

  if (controller_) {
    controller_->terminate();
    controller_ = nullptr;
  }
  if (component_) {
    component_->terminate();
    component_ = nullptr;
  }

  processor_ = nullptr;
  view_ = nullptr;
  module_ = nullptr;

  return absl::OkStatus();
}

absl::Status VstPlugin::Init(const std::string& path, const std::string& uid,
                             FUnknown* host_context) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string error;
  module_ = VST3::Hosting::Module::create(path, error);
  if (!module_) {
    return absl::InternalError("Failed to load VST module: " + error);
  }

  auto factory = module_->getFactory();
  if (!factory.get()) {
    return absl::InternalError("Failed to get plugin factory");
  }

  for (auto& info : factory.classInfos()) {
    if (info.category() == kVstAudioEffectClass) {
      component_ = factory.createInstance<IComponent>(info.ID());
      if (component_) {
        break;
      }
    }
  }

  if (!component_) {
    return absl::NotFoundError("No audio effect component found in plugin");
  }

  auto result = component_->initialize(host_context);
  if (result != kResultOk) {
    return absl::InternalError("Failed to initialize component");
  }

  processor_ = FUnknownPtr<IAudioProcessor>(component_);
  if (!processor_) {
    return absl::InternalError("Component does not support audio processing");
  }

  TUID controller_cid;
  if (component_->getControllerClassId(controller_cid) == kResultOk) {
    controller_ = factory.createInstance<IEditController>(controller_cid);
    if (controller_) {
      controller_->initialize(host_context);
    }
  }

  if (!controller_) {
    controller_ = FUnknownPtr<IEditController>(component_);
  }

  LOG(INFO) << "Loaded VST plugin: " << path;
  return absl::OkStatus();
}

absl::Status VstPlugin::Activate(int sample_rate, int block_size) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (activated_) {
    return absl::OkStatus();
  }

  process_setup_.processMode = kRealtime;
  process_setup_.symbolicSampleSize = kSample32;
  process_setup_.maxSamplesPerBlock = block_size;
  process_setup_.sampleRate = sample_rate;

  auto result = processor_->setupProcessing(process_setup_);
  if (result != kResultOk) {
    return absl::InternalError("Failed to setup processing");
  }

  result = component_->setActive(true);
  if (result != kResultOk) {
    return absl::InternalError("Failed to activate component");
  }

  input_left_.resize(block_size);
  input_right_.resize(block_size);
  output_left_.resize(block_size);
  output_right_.resize(block_size);

  input_ptrs_[0] = input_left_.data();
  input_ptrs_[1] = input_right_.data();
  output_ptrs_[0] = output_left_.data();
  output_ptrs_[1] = output_right_.data();

  input_buffers_.numChannels = 2;
  input_buffers_.channelBuffers32 = input_ptrs_;
  input_buffers_.silenceFlags = 0;

  output_buffers_.numChannels = 2;
  output_buffers_.channelBuffers32 = output_ptrs_;
  output_buffers_.silenceFlags = 0;

  process_data_.processMode = kRealtime;
  process_data_.symbolicSampleSize = kSample32;
  process_data_.numSamples = block_size;
  process_data_.numInputs = 1;
  process_data_.numOutputs = 1;
  process_data_.inputs = &input_buffers_;
  process_data_.outputs = &output_buffers_;

  activated_ = true;
  LOG(INFO) << "VST plugin activated at " << sample_rate << " Hz, "
            << block_size << " samples";

  return absl::OkStatus();
}

absl::Status VstPlugin::Deactivate() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!activated_) {
    return absl::OkStatus();
  }

  if (component_) {
    component_->setActive(false);
  }

  activated_ = false;
  LOG(INFO) << "VST plugin deactivated";

  return absl::OkStatus();
}

void VstPlugin::Process(AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!activated_ || !processor_) {
    return;
  }

  auto* left_in = buffer.GetChannel(kLeftChannel);
  auto* right_in = buffer.GetChannel(kRightChannel);
  auto size = buffer.Size();

  std::copy(left_in, left_in + size, input_left_.begin());
  std::copy(right_in, right_in + size, input_right_.begin());

  process_data_.numSamples = size;
  processor_->process(process_data_);

  std::copy(output_left_.begin(), output_left_.begin() + size, left_in);
  std::copy(output_right_.begin(), output_right_.begin() + size, right_in);
}

std::map<std::string, VstParameter> VstPlugin::GetParameters() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::map<std::string, VstParameter> params;

  if (!controller_) {
    return params;
  }

  int32 count = controller_->getParameterCount();
  for (int32 i = 0; i < count; ++i) {
    ParameterInfo info;
    if (controller_->getParameterInfo(i, info) == kResultOk) {
      VstParameter param;
      param.id = info.id;
      param.name = VST3::StringConvert::convert(info.title);
      param.short_name = VST3::StringConvert::convert(info.shortTitle);
      param.default_value = info.defaultNormalizedValue;
      param.min_value = 0.0f;
      param.max_value = 1.0f;
      param.step_count = info.stepCount;

      params[param.name] = param;
    }
  }

  return params;
}

absl::Status VstPlugin::SetParameter(uint32_t id, float value) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!controller_) {
    return absl::FailedPreconditionError("No edit controller available");
  }

  auto result = controller_->setParamNormalized(id, value);
  if (result != kResultOk) {
    return absl::InternalError("Failed to set parameter");
  }

  return absl::OkStatus();
}

absl::StatusOr<float> VstPlugin::GetParameter(uint32_t id) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!controller_) {
    return absl::FailedPreconditionError("No edit controller available");
  }

  return controller_->getParamNormalized(id);
}

bool VstPlugin::HasEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!controller_) {
    return false;
  }

  auto view = controller_->createView(ViewType::kEditor);
  return view != nullptr;
}

absl::Status VstPlugin::OpenEditor(void* parent_window) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (editor_open_) {
    return absl::AlreadyExistsError("Editor already open");
  }

  if (!controller_) {
    return absl::FailedPreconditionError("No edit controller available");
  }

  view_ = controller_->createView(ViewType::kEditor);
  if (!view_) {
    return absl::NotFoundError("Plugin does not have an editor");
  }

  auto result = view_->attached(parent_window, kPlatformTypeNSView);
  if (result != kResultOk) {
    view_ = nullptr;
    return absl::InternalError("Failed to attach editor view");
  }

  editor_open_ = true;
  return absl::OkStatus();
}

absl::Status VstPlugin::CloseEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!editor_open_) {
    return absl::OkStatus();
  }

  if (view_) {
    view_->removed();
    view_ = nullptr;
  }

  editor_open_ = false;
  return absl::OkStatus();
}

bool VstPlugin::IsEditorOpen() {
  std::lock_guard<std::mutex> lock(mutex_);
  return editor_open_;
}

absl::StatusOr<std::vector<uint8_t>> VstPlugin::SaveState() {
  std::lock_guard<std::mutex> lock(mutex_);

  // TODO: Implement state saving using IComponent::getState()
  return std::vector<uint8_t>();
}

absl::Status VstPlugin::LoadState(const std::vector<uint8_t>& state) {
  std::lock_guard<std::mutex> lock(mutex_);

  // TODO: Implement state loading using IComponent::setState()
  return absl::OkStatus();
}

}  // namespace vst
}  // namespace soir
