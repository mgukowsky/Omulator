#pragma once

#include "omulator/vkmisc/Allocator.hpp"
#include "omulator/vkmisc/vkmisc.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace omulator::vkmisc {

/**
 * Class which represents CPU local data which can be updated and sent to the GPU
 */
template<typename T>
class GPUBuffer {
public:
  /**
   * N.B. that size is the number of T elements to allocate (not bytes).
   */
  GPUBuffer(ILogger                      &logger,
            Allocator                    &allocator,
            const std::size_t             size,
            const vk::BufferUsageFlagBits usage,
            const Allocator::Residency    residency)
    : pLogger_(&logger),
      pAllocator_(&allocator),
      size_(size),
      uploadSize_(size_ * sizeof(T)),
      isValid_(true) {
    localBuff_.resize(size);

    // TODO: Can calculating the size of the vector in bytes in this manner omit any additional
    // alignment bytes?
    vk::BufferCreateInfo createInfo({}, uploadSize_, usage);
    allocation_ = pAllocator_->alloc(createInfo, residency);
  }

  ~GPUBuffer() {
    if(isValid_) {
      pAllocator_->free(allocation_);
    }
  }

  // In the case of Allocator using VMA, then the allocation will point to a mutex which CANNOT be
  // freed prematurely, so we have to implement custom move semantics to handle this
  GPUBuffer(const GPUBuffer &)            = delete;
  GPUBuffer &operator=(const GPUBuffer &) = delete;

  GPUBuffer(GPUBuffer &&rhs) { *this = std::move(rhs); }

  GPUBuffer &operator=(GPUBuffer &&rhs) {
    if(this != &rhs) {
      pLogger_    = rhs.pLogger_;
      pAllocator_ = rhs.pAllocator_;
      allocation_ = std::move(rhs.allocation_);
      localBuff_  = std::move(rhs.localBuff_);
      size_       = rhs.size_;
      uploadSize_ = rhs.uploadSize_;
      isValid_    = true;

      rhs.isValid_                = false;
      rhs.pLogger_                = nullptr;
      rhs.pAllocator_             = nullptr;
      rhs.allocation_.handle      = nullptr;
      rhs.allocation_.pAllocation = nullptr;
    }

    return *this;
  }

  vk::Buffer &buff() {
    assert(isValid_);
    return allocation_.handle;
  }

  /**
   * Returns a reference to the CPU-local data. The data can be mutated and then sent to the GPU
   * with upload().
   */
  std::vector<T> &data() {
    assert(isValid_);
    return localBuff_;
  }

  std::size_t size() const noexcept {
    assert(isValid_);
    return localBuff_.size();
  }

  /**
   * Send the data in localBuff_ to the GPU.
   */
  void upload() {
    assert(isValid_);
    assert(localBuff_.size() == size_
           && "The size of localBuff_ should match the size of the data allocated on the GPU when "
              "GPUBuffer's constructor runs, at least until a function such as "
              "vkmisc::Allocator::realloc is implemented");

    // TODO: if the size in the assert doesn't match, call vkmisc::Allocator::realloc to ensure that
    // the size of the GPU memory matches the size of the data we're about to send to it.
    pAllocator_->upload(allocation_, localBuff_.data(), uploadSize_);
  }

private:
  ILogger   *pLogger_;
  Allocator *pAllocator_;

  AllocatedBuffer allocation_;
  std::vector<T>  localBuff_;

  std::size_t size_;
  std::size_t uploadSize_;

  bool isValid_;
};

}  // namespace omulator::vkmisc