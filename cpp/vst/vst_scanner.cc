#include "vst/vst_scanner.hh"

#include <absl/log/log.h>

#include <filesystem>

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/module.h"

namespace soir {
namespace vst {

namespace fs = std::filesystem;

VstScanner::VstScanner() : search_paths_(GetDefaultSearchPaths()) {}

std::vector<std::string> VstScanner::GetDefaultSearchPaths() {
  std::vector<std::string> paths;

#if defined(__APPLE__)
  paths.push_back("/Library/Audio/Plug-Ins/VST3");

  const char* home = std::getenv("HOME");
  if (home) {
    paths.push_back(std::string(home) + "/Library/Audio/Plug-Ins/VST3");
  }
#elif defined(_WIN32)
  paths.push_back("C:\\Program Files\\Common Files\\VST3");
  paths.push_back("C:\\Program Files (x86)\\Common Files\\VST3");
#elif defined(__linux__)
  paths.push_back("/usr/lib/vst3");
  paths.push_back("/usr/local/lib/vst3");

  const char* home = std::getenv("HOME");
  if (home) {
    paths.push_back(std::string(home) + "/.vst3");
  }
#endif

  return paths;
}

void VstScanner::AddSearchPath(const std::string& path) {
  search_paths_.push_back(path);
}

absl::Status VstScanner::Scan(std::map<std::string, PluginInfo>* plugins) {
  for (const auto& path : search_paths_) {
    auto status = ScanDirectory(path, plugins);
    if (!status.ok()) {
      LOG(WARNING) << "Failed to scan directory " << path << ": " << status;
    }
  }

  return absl::OkStatus();
}

absl::Status VstScanner::ScanDirectory(
    const std::string& path, std::map<std::string, PluginInfo>* plugins) {
  if (!fs::exists(path)) {
    return absl::OkStatus();
  }

  for (const auto& entry : fs::recursive_directory_iterator(path)) {
    if (entry.is_directory() && entry.path().extension() == ".vst3") {
      auto status = ProbePlugin(entry.path().string(), plugins);
      if (!status.ok()) {
        LOG(WARNING) << "Failed to probe plugin " << entry.path() << ": "
                     << status;
      }
    }
  }

  return absl::OkStatus();
}

absl::Status VstScanner::ProbePlugin(
    const std::string& bundle_path,
    std::map<std::string, PluginInfo>* plugins) {
  std::string error;
  auto module = VST3::Hosting::Module::create(bundle_path, error);
  if (!module) {
    return absl::InternalError("Failed to load module: " + error);
  }

  auto factory = module->getFactory();
  if (!factory.get()) {
    return absl::InternalError("Failed to get factory");
  }

  for (auto& info : factory.classInfos()) {
    if (info.category() == kVstAudioEffectClass) {
      PluginInfo plugin_info;

      plugin_info.uid = info.ID().toString();

      plugin_info.name = info.name();
      plugin_info.vendor = info.vendor();

      // Join subcategories with "|"
      auto subcats = info.subCategories();
      std::string categories;
      for (size_t i = 0; i < subcats.size(); ++i) {
        if (i > 0) {
          categories += "|";
        }
        categories += subcats[i];
      }
      plugin_info.category = categories;
      plugin_info.path = bundle_path;
      plugin_info.num_audio_inputs = 2;
      plugin_info.num_audio_outputs = 2;

      (*plugins)[plugin_info.name] = plugin_info;
      LOG(INFO) << "Found VST plugin: " << plugin_info.name << " ("
                << plugin_info.vendor << ")";
    }
  }

  return absl::OkStatus();
}

}  // namespace vst
}  // namespace soir
