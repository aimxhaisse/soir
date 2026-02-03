#pragma once

#include <absl/status/status.h>

#include <map>
#include <string>
#include <vector>

#include "vst/vst_host.hh"

namespace soir {
namespace vst {

class VstScanner {
 public:
  VstScanner();

  absl::Status Scan(std::map<std::string, PluginInfo>* plugins);

  void AddSearchPath(const std::string& path);

 private:
  std::vector<std::string> GetDefaultSearchPaths();
  absl::Status ScanDirectory(const std::string& path,
                             std::map<std::string, PluginInfo>* plugins);
  absl::Status ProbePlugin(const std::string& bundle_path,
                           std::map<std::string, PluginInfo>* plugins);

  std::vector<std::string> search_paths_;
};

}  // namespace vst
}  // namespace soir
