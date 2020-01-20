#include "omulator/CPUIdentifier.hpp"

#include "omulator/util/intrinsics.hpp"

namespace omulator {

class CPUIdentifier::CPUIdentifier_impl {
public:
  bool is_cpu_supported() const noexcept {
#if defined(OML_ARCH_X64)
    return __builtin_cpu_supports("avx2");
#elif defined(OML_ARCH_ARM)
#error CPUIdentifier_impl must be defined for the ARM architecture
#else
#error Unknown architecture
#endif
  }
}; /* class CPUIdentifier_impl */

// Required for pImpl to work
CPUIdentifier::CPUIdentifier()  = default;
CPUIdentifier::~CPUIdentifier() = default;

bool CPUIdentifier::is_cpu_supported() const noexcept { return impl_->is_cpu_supported(); }

} /* namespace omulator */
