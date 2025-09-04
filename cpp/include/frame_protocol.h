#pragma once
#include <cstdint>
#include <cstring>
#include <chrono>

namespace acq {

static constexpr std::uint64_t FRAME_MAGIC = 0x0AD51256AD51256AULL;

struct FrameHeader {
    std::uint64_t magic { FRAME_MAGIC };      // Identificador
    std::uint64_t timestamp_ns { 0 };         // Epoch ns do primeiro sample
    std::uint32_t channels { 0 };             // Nº canais
    std::uint32_t samples_per_channel { 0 };  // Amostras por canal no frame
    std::uint32_t sample_rate_hz { 0 };       // Taxa nominal
    std::uint32_t reserved { 0 };             // Reservado (0)
};

static_assert(sizeof(FrameHeader) == 40, "FrameHeader deve ter 40 bytes");

inline std::uint64_t now_epoch_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace acq
