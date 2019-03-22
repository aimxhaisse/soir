#include "mods/debug.h"

namespace soir {

ModDebug::ModDebug(Context &ctx) : Mod(ctx) {}

Status ModDebug::Init(const Config &config) { return StatusCode::OK; }

void ModDebug::Render() {}

} // namespace soir
