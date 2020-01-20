#pragma once

#include <atomic>
#include <cstdint>

class Dummy {
public:
  Dummy() { ++numInstances; }

  ~Dummy() { --numInstances; }

  static inline std::atomic_size_t numInstances = 0;

  static void reset() { numInstances = 0; }
};
