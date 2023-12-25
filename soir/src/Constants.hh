#pragma once

namespace maethstro {

constexpr double kSampleRate = 44100;
constexpr int kBlockSize = 44100 / 2;
constexpr int kStereo = 2;
constexpr int kBitsPerSample = 24;
constexpr float kVorbisQuality = 0.0;
constexpr const char* kPluginTypeSampler = "sampler";
constexpr const char* kPluginTypeVst3 = "vst3";

}  // namespace maethstro
