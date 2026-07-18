#include "vst/vst_plugin.hh"

#include <absl/log/log.h>

#include <libremidi/message.hpp>

#include "core/midi_event.hh"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
namespace soir {
namespace vst {

using namespace Steinberg;
using namespace Steinberg::Vst;

// EventList implementation.

int32 PLUGIN_API EventList::getEventCount() { return events_.size(); }

tresult PLUGIN_API EventList::getEvent(int32 index, Event& e) {
  if (index < 0 || index >= static_cast<int32>(events_.size())) {
    return kInvalidArgument;
  }
  e = events_[index];
  return kResultOk;
}

tresult PLUGIN_API EventList::addEvent(Event& e) {
  events_.push_back(e);
  return kResultOk;
}

void EventList::Clear() { events_.clear(); }

tresult PLUGIN_API EventList::queryInterface(const TUID iid, void** obj) {
  if (FUnknownPrivate::iidEqual(iid, IEventList::iid) ||
      FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
    *obj = this;
    addRef();
    return kResultOk;
  }
  *obj = nullptr;
  return kNoInterface;
}

uint32 PLUGIN_API EventList::addRef() { return ++ref_count_; }

uint32 PLUGIN_API EventList::release() {
  auto count = --ref_count_;
  return count;
}

VstPlugin::VstPlugin()
    : activated_(false),
      editor_open_(false),
      input_ptrs_{nullptr, nullptr},
      output_ptrs_{nullptr, nullptr} {}

VstPlugin::~VstPlugin() { Shutdown().IgnoreError(); }

absl::Status VstPlugin::Shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  // Per VST3 spec, call removed() before releasing the view. The bridge
  // process may have already exited (e.g. X11 error killed it), in which
  // case removed() throws std::system_error (EOF on the socket). Catch it
  // here so the exception never escapes into the noexcept destructor chain.
  if (view_) {
    auto view = std::move(view_);
    editor_open_ = false;
    try {
      view->setFrame(nullptr);
      view->removed();
    } catch (const std::exception& e) {
      LOG(WARNING) << "VST view removal failed (bridge may have exited): "
                   << e.what();
    } catch (...) {
      LOG(WARNING) << "VST view removal failed with unknown exception";
    }
  }
  plug_frame_ = nullptr;

  if (activated_) {
    if (processor_) {
      processor_->setProcessing(false);
    }
    if (component_) {
      component_->setActive(false);
    }
    activated_ = false;
  }

  // Per VST3 spec, disconnect connection points before calling terminate().
  if (component_ && controller_) {
    FUnknownPtr<IConnectionPoint> comp_cp(component_);
    FUnknownPtr<IConnectionPoint> ctrl_cp(controller_);
    if (comp_cp && ctrl_cp) {
      comp_cp->disconnect(ctrl_cp);
      ctrl_cp->disconnect(comp_cp);
    }
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
  module_ = nullptr;

  return absl::OkStatus();
}

absl::Status VstPlugin::Init(const std::string& path, const std::string& uid,
                             FUnknown* host_context, VstPluginType type) {
  std::lock_guard<std::mutex> lock(mutex_);
  type_ = type;

  std::string error;
  module_ = VST3::Hosting::Module::create(path, error);
  if (!module_) {
    return absl::InternalError("Failed to load VST module: " + error);
  }

  auto factory = module_->getFactory();
  if (!factory.get()) {
    return absl::InternalError("Failed to get plugin factory");
  }

  // Pass the host context to the plugin factory so VSTGUI-based plug-ins on
  // Linux can retrieve the IRunLoop needed for their event loop integration.
  // IPluginFactory3::setHostContext forwards the context to callbacks
  // registered during module load (e.g. setupVSTGUIRunloop) and on Linux
  // also stores it for the platform timer implementation.
  FUnknownPtr<IPluginFactory3> factory3(factory.get());
  if (factory3) {
    factory3->setHostContext(host_context);
  }

  for (auto& info : factory.classInfos()) {
    if (info.category() == kVstAudioEffectClass) {
      if (info.ID().toString() == uid) {
        component_ = factory.createInstance<IComponent>(info.ID());
        if (component_) {
          break;
        }
      }
    }
  }

  if (!component_) {
    return absl::NotFoundError("No audio effect component found in plugin");
  }

#ifdef __linux__
  // Note: we intentionally do NOT use RTLD_NODELETE here. While it prevents
  // use-after-free in bridge libraries (yabridge), it also keeps VSTGUI's
  // global destructor registered until process exit, where it can run after
  // the X11 connection is gone. By allowing dlclose() to unload the plugin,
  // the VSTGUI destructor runs during Shutdown() instead, while the display
  // connection is still valid.
#endif

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

  // Connect component and controller if they are separate objects.
  if (controller_) {
    FUnknownPtr<IConnectionPoint> comp_cp(component_);
    FUnknownPtr<IConnectionPoint> ctrl_cp(controller_);
    if (comp_cp && ctrl_cp) {
      comp_cp->connect(ctrl_cp);
      ctrl_cp->connect(comp_cp);
    }
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

  bool is_instrument = (type_ == VstPluginType::kVstInstrument);

  component_->activateBus(MediaTypes::kAudio, BusDirections::kOutput, 0, true);
  if (is_instrument) {
    component_->activateBus(MediaTypes::kEvent, BusDirections::kInput, 0, true);
  } else {
    component_->activateBus(MediaTypes::kAudio, BusDirections::kInput, 0, true);
  }

  result = component_->setActive(true);
  if (result != kResultOk && result != kNotImplemented) {
    return absl::InternalError("Failed to activate component");
  }

  // setProcessing is optional per the VST3 spec — plug-ins that don't need to
  // distinguish active-but-idle from actively-processing return kNotImplemented
  // (e.g. the SDK's AGain sample). Treat both that and kResultOk as success.
  result = processor_->setProcessing(true);
  if (result != kResultOk && result != kNotImplemented) {
    return absl::InternalError("Failed to start processing");
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
  process_data_.numInputs = is_instrument ? 0 : 1;
  process_data_.numOutputs = 1;
  process_data_.inputs = is_instrument ? nullptr : &input_buffers_;
  process_data_.outputs = &output_buffers_;
  process_data_.inputEvents = &input_events_;
  process_data_.outputEvents = &output_events_;
  // The VST3 spec allows process data to carry parameter changes. Several
  // plug-in frameworks assert that these pointers are non-null. Provide
  // empty queues and a minimal process context so plug-ins can read the
  // sample rate if they need it.
  process_data_.inputParameterChanges = &input_param_changes_;
  process_data_.outputParameterChanges = &output_param_changes_;
  process_context_.sampleRate = sample_rate;
  process_data_.processContext = &process_context_;

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

  processor_->setProcessing(false);

  if (component_) {
    component_->setActive(false);
  }

  activated_ = false;
  LOG(INFO) << "VST plugin deactivated";

  return absl::OkStatus();
}

void VstPlugin::PopulateEventList(SampleTick block_start_tick,
                                  const std::list<MidiEventAt>& events) {
  input_events_.Clear();
  output_events_.Clear();

  for (const auto& midi_event : events) {
    const auto& msg = midi_event.Msg();
    if (msg.bytes.empty()) {
      continue;
    }

    Event vst_event{};
    vst_event.busIndex = 0;
    vst_event.ppqPosition = 0;
    vst_event.flags = Event::kIsLive;

    SampleTick offset = midi_event.Tick() - block_start_tick;
    if (offset < 0) {
      offset = 0;
    }
    vst_event.sampleOffset = static_cast<int32>(offset);

    auto status = msg.get_message_type();
    auto channel = static_cast<int16>(msg.get_channel());

    if (status == libremidi::message_type::NOTE_ON) {
      vst_event.type = Event::kNoteOnEvent;
      vst_event.noteOn.channel = channel;
      vst_event.noteOn.pitch = static_cast<int16>(msg.bytes[1]);
      vst_event.noteOn.velocity = static_cast<float>(msg.bytes[2]) / 127.0f;
      vst_event.noteOn.tuning = 0.0f;
      vst_event.noteOn.length = 0;
      vst_event.noteOn.noteId = -1;
      input_events_.addEvent(vst_event);
    } else if (status == libremidi::message_type::NOTE_OFF) {
      vst_event.type = Event::kNoteOffEvent;
      vst_event.noteOff.channel = channel;
      vst_event.noteOff.pitch = static_cast<int16>(msg.bytes[1]);
      vst_event.noteOff.velocity = static_cast<float>(msg.bytes[2]) / 127.0f;
      vst_event.noteOff.noteId = -1;
      vst_event.noteOff.tuning = 0.0f;
      input_events_.addEvent(vst_event);
    } else if (status == libremidi::message_type::POLY_PRESSURE) {
      vst_event.type = Event::kPolyPressureEvent;
      vst_event.polyPressure.channel = channel;
      vst_event.polyPressure.pitch = static_cast<int16>(msg.bytes[1]);
      vst_event.polyPressure.pressure =
          static_cast<float>(msg.bytes[2]) / 127.0f;
      vst_event.polyPressure.noteId = -1;
      input_events_.addEvent(vst_event);
    }
  }
}

void VstPlugin::Process(SampleTick tick, AudioBuffer& buffer,
                        const std::list<MidiEventAt>& events) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!activated_ || !processor_) {
    return;
  }

  auto* left_in = buffer.GetChannel(kLeftChannel);
  auto* right_in = buffer.GetChannel(kRightChannel);
  auto size = buffer.Size();

  bool is_instrument = (type_ == VstPluginType::kVstInstrument);

  if (!is_instrument) {
    std::copy(left_in, left_in + size, input_left_.begin());
    std::copy(right_in, right_in + size, input_right_.begin());
  }

  PopulateEventList(tick, events);

  process_data_.numSamples = size;
  processor_->process(process_data_);

  // Discard any parameter changes the plug-in emitted; we do not surface
  // automated VST parameters back to the host. Clearing per-block keeps the
  // queue from growing unboundedly.
  output_param_changes_.clearQueue();

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

std::pair<int, int> VstPlugin::GetEditorSize() const { return editor_size_; }

bool VstPlugin::HasEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!controller_) {
    return false;
  }

  // createView transfers ownership of a new reference; adopt it so the view
  // is released instead of leaked.
  Steinberg::IPtr<Steinberg::IPlugView> view =
      Steinberg::owned(controller_->createView(ViewType::kEditor));
  return view != nullptr;
}

absl::Status VstPlugin::OpenEditor(void* parent_window) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!controller_) {
    return absl::FailedPreconditionError("No edit controller available");
  }

  // If the view is already attached, just mark it as open. The caller handles
  // showing the X11 window. Reusing the existing view avoids spawning a second
  // Wine/bridge process before the first has finished cleanup, which races on
  // shared X11 resources and causes BadWindow → socket EOF → std::system_error
  // in a yabridge background thread → terminate().
  if (view_) {
    editor_open_ = true;
    return absl::OkStatus();
  }

  view_ = controller_->createView(ViewType::kEditor);
  if (!view_) {
    return absl::NotFoundError("Plugin does not have an editor");
  }

  // Per VST3 spec, the host must provide an IPlugFrame before attaching the
  // view so the plug-in can request resizes. Several frameworks assert on this
  // and refuse to attach if no frame is set.
  if (!plug_frame_) {
    plug_frame_ = owned(CreateHostPlugFrame().release());
  }
  view_->setFrame(plug_frame_.get());

  ViewRect rect{};
  if (view_->getSize(&rect) == kResultOk) {
    editor_size_ = {rect.right - rect.left, rect.bottom - rect.top};
  }

  auto result = AttachEditorView(view_.get(), parent_window);
  if (result != kResultOk) {
    view_->setFrame(nullptr);
    view_ = nullptr;
    return absl::InternalError("Failed to attach editor view");
  }

  editor_open_ = true;
  return absl::OkStatus();
}

absl::Status VstPlugin::CloseEditor() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Do not call view_->removed() here: that tears down the Wine/bridge process,
  // which may still be cleaning up when the editor is reopened, causing a race
  // on X11 resources. The view stays attached; Shutdown() calls removed() when
  // the plugin is truly being destroyed.
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
