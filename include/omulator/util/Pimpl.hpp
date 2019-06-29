#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace omulator::util {

/**
 * An implementation of the pImpl pattern, as recommended by Herb
 * Sutter (https://herbsutter.com/gotw/_101/).
 *
 * N.B. this is the "non-fast" version of pImpl. There are more efficient
 * versions of this pattern which avoid the heap allocation, and help with
 * data locality, at the cost of needing to calculate/guess the storage 
 * size yourself ;) More info here:
 * https://www.cleeus.de/w/blog/2017/03/10/static_pimpl_idiom.html
 *
 * @param T the underlying type. MUST have its constructor(s) and destructor
 *    declared and defined!
 */
template<typename T>
class Pimpl {
public:
  template<typename ...Args>
  explicit Pimpl(Args &&...args)
    : pimpl_(std::make_unique<T>(std::forward<Args>(args)...))
  {

  }

  ~Pimpl() = default;

  //Copying would not be enabled anyway because unique_ptr is move only
  Pimpl(const Pimpl&) = delete;
  Pimpl& operator=(const Pimpl&) = delete;

  //Moving is fine
  Pimpl(Pimpl&&) noexcept = default;
  Pimpl& operator=(Pimpl&&) noexcept = default;

  //Functions enabling this class to act as a pointer proxy
  inline T* operator->() noexcept { return pimpl_.get(); }
  inline T& operator*() noexcept { return *pimpl_.get(); }

  inline const T* operator->() const noexcept { return pimpl_.get(); }
  inline const T& operator*() const noexcept { return *pimpl_.get(); }

private:
  std::unique_ptr<T> pimpl_;
};

} /* namespace omulator::util */