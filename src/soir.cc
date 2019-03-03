#include "config.h"

int main() {
  std::unique_ptr<soir::Config> config =
      soir::Config::LoadFromPath("/dev/null");

  return 42;
}
