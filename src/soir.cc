#include "config.h"

using namespace soir;

int main() {
  StatusOr<std::unique_ptr<Config>> config = Config::LoadFromPath("/dev/null");

  return 42;
}
