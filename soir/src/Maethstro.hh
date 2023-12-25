#pragma once

#include "Config.hh"
#include "ConfigParser.hh"
#include "LiquidPusher.hh"
#include "NetworkService.hh"
#include "Renderer.hh"

namespace maethstro {

struct Maethstro {
  Status Init(const std::string& path);
  Status Run();

 private:
  std::unique_ptr<ConfigParser> configParser_;
  std::unique_ptr<Config> config_;
  std::unique_ptr<NetworkService> service_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<LiquidPusher> liquidifier_;
};

}  // namespace maethstro
