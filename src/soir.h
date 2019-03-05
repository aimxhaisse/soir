#ifndef SOIR_H
#define SOIR_H

#include "config.h"
#include "status.h"

namespace soir {

class Soir {
public:
  Status Init();
  Status Run();

private:
  std::unique_ptr<Config> core_config_;
};

} // namespace soir

#endif // SOIR_H
