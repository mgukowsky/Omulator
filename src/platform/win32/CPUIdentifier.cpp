#include "omulator/CPUIdentifier.hpp"

#include "omulator/util/intrinsics.hpp"

#include <array>
#include <vector>

namespace omulator {

class CPUIdentifier::CPUIdentifier_impl {
public:
  bool is_cpu_supported() const noexcept {

#if defined(OML_ARCH_X64)
    //Checks for AVX2 support
    std::array<int, 4> cpuinfo;
    __cpuidex(cpuinfo.data(), 0x07, 0);
    return cpuinfo[1] & 0x0020;
#elif defined(OML_ARCH_ARM)
#error CPUIdentifier_impl must be defined for the ARM architecture
#else
#error Unknown architecture
#endif

  }
}; /* class CPUIdentifier_impl */

//Must be defined here for pImpl to work
CPUIdentifier::CPUIdentifier() = default;
CPUIdentifier::~CPUIdentifier() = default;

bool CPUIdentifier::is_cpu_supported() const noexcept {
  return impl_->is_cpu_supported();
}

} /* namespace omulator */
