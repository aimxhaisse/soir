#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp/controls.hh"
#include "soir.grpc.pb.h"

namespace soir {
namespace dsp {

Knob::Knob() {}

void Knob::SetTargetValue(SampleTick tick, float target) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  initialValue_ = targetValue_;
  targetValue_ = target;

  fromTick_ = tick;
  toTick_ = tick + kSampleRate / kControlsFrequencyUpdate;
}

float Knob::GetValue(SampleTick tick) {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  if (tick >= toTick_) {
    return targetValue_;
  }

  const float progress = (tick - fromTick_) / (toTick_ - fromTick_);

  return initialValue_ + (targetValue_ - initialValue_) * progress;
}

absl::Status Controls::Init() {
  return absl::OkStatus();
}

Controls::Controls() {}

Knob* Controls::GetControl(const std::string& id) {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  auto it = controls_.find(id);
  if (it == controls_.end()) {
    return nullptr;
  }

  return it->second.get();
}

void Controls::AddEvents(const std::list<MidiEventAt>& events) {
  midi_stack_.AddEvents(events);
}

void Controls::Update(SampleTick current) {
  std::list<MidiEventAt> events;

  midi_stack_.EventsAtTick(current, events);

  for (auto& event : events) {
    ProcessEvent(event);
  }
}

void Controls::ProcessEvent(MidiEventAt& event_at) {
  auto msg = event_at.Msg();
  auto type = msg.get_message_type();

  if (type != libremidi::message_type::SYSTEM_EXCLUSIVE) {
    return;
  }

  // Parse the sysex message.
  proto::MidiSysexInstruction sysex;
  if (!sysex.ParseFromArray(msg.bytes.data() + 1, msg.bytes.size() - 1)) {
    LOG(WARNING) << "Failed to parse sysex message in controls update";
    return;
  }
  if (sysex.type() != proto::MidiSysexInstruction::UPDATE_CONTROLS) {
    return;
  }

  // Extract values to update from the JSON.
  std::map<std::string, float> values;
  rapidjson::Document params;
  params.Parse(sysex.json_payload().c_str());
  for (auto& k : params["knobs"].GetObject()) {
    values[k.name.GetString()] = k.value.GetDouble();
  }

  // Only take the lock at the last moment.
  std::unique_lock<std::shared_mutex> lock(mutex_);

  for (auto& v : values) {
    auto& name = v.first;
    auto target = v.second;
    auto it = controls_.find(name);
    if (it == controls_.end()) {
      it = controls_.emplace(name, std::make_unique<Knob>()).first;
    }

    it->second->SetTargetValue(event_at.Tick(), target);
  }

  // Here eventually we could GC all names that weren't found. It's
  // not clear though how we can properly handle this since we need
  // the DSP code to ack it doesn't use any legacy knob that we are
  // deleting.
}

}  // namespace dsp
}  // namespace soir
