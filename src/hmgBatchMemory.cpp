#include <cstddef>
#include <algorithm>
#include <limits>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace nsHMG {
std::size_t opticalAttenuationCacheBudget() noexcept {
#ifdef _WIN32
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(GlobalMemoryStatusEx(&memory))
        return static_cast<std::size_t>(std::min<unsigned long long>(
            memory.ullAvailPhys/3,std::numeric_limits<std::size_t>::max()));
#endif
    return 0;
}
}
