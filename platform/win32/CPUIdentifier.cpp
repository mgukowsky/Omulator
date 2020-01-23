#include "omulator/CPUIdentifier.hpp"

#include "omulator/util/intrinsics.hpp"

#include <array>
#include <vector>

namespace omulator {

class CPUIdentifier::CPUIdentifier_impl {
public:
  static bool is_cpu_supported() noexcept {
#if defined(OML_ARCH_X64)

    // See MSVC and Intel's documentation for the CPUID instruction for an explanation
    // of what these numbers refer to.
    constexpr int AVX2_BIT             = 0x0020;
    constexpr std::size_t CPUID_BLOCKS = 4u;
    constexpr int EX_FEATURE_LEAF_FUNC = 0x07;

    std::array<int, CPUID_BLOCKS> cpuinfo;
    __cpuidex(cpuinfo.data(), EX_FEATURE_LEAF_FUNC, 0);
    return cpuinfo[1] & AVX2_BIT;
#elif defined(OML_ARCH_ARM)
#error CPUIdentifier_impl must be defined for the ARM architecture
#else
#error Unknown architecture
#endif
  }
}; /* class CPUIdentifier_impl */

// Must be defined here for pImpl to work
CPUIdentifier::CPUIdentifier()  = default;
CPUIdentifier::~CPUIdentifier() = default;

bool CPUIdentifier::is_cpu_supported() noexcept { return CPUIdentifier_impl::is_cpu_supported(); }

} /* namespace omulator */
