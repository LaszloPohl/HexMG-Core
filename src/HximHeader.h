#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace simulator::results {
#pragma pack(push, 1)
struct HximHeaderV1 {
    char magic[4] = {'H', 'X', 'I', 'M'};
    std::uint8_t version = 1;
    std::uint8_t flags = 0;
    std::uint8_t analysisKind = 1;
    std::uint8_t reserved0 = 0;
    double analysisExtra1 = 0;
    double analysisExtra2 = 0;
    float analysisExtra3 = 0;
    std::uint32_t stepOrdinal = 0;
    std::uint8_t gridId[16] = {};
    std::uint8_t runId[16] = {};
    std::uint8_t reserved[16] = {};
};
#pragma pack(pop)
static_assert(std::is_trivially_copyable_v<HximHeaderV1>);
static_assert(std::is_standard_layout_v<HximHeaderV1>);
static_assert(sizeof(HximHeaderV1) == 80);
static_assert(alignof(HximHeaderV1) == 1);
static_assert(offsetof(HximHeaderV1, analysisExtra1) == 8);
static_assert(offsetof(HximHeaderV1, analysisExtra2) == 16);
static_assert(offsetof(HximHeaderV1, analysisExtra3) == 24);
static_assert(offsetof(HximHeaderV1, stepOrdinal) == 28);
static_assert(offsetof(HximHeaderV1, gridId) == 32);
static_assert(offsetof(HximHeaderV1, runId) == 48);
static_assert(offsetof(HximHeaderV1, reserved) == 64);
static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559);
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
// Direct packed-struct input is supported only on little-endian hosts.
static_assert(std::endian::native==std::endian::little);
inline constexpr std::uint8_t hximDouble=1, hximComplex=2, hximCurrent=4;
}
