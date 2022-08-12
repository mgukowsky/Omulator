/**
 * We use this file to isolate injector recipes, and the implied dependencies, to a single
 * translation unit.
 */

#include "omulator/Clock.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/SpdlogLogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/di/TypeHash.hpp"

#include <map>
#include <memory_resource>
#include <thread>

using omulator::di::TypeHash;

namespace omulator::di {
void Injector::installDefaultRules(Injector &injector) {
  /**
   * Constructor recipes should be added here. N.B. that types which can be default-initialized DO
   * NOT need to be listed here!
   */

  /**
   * Implementations should be bound to interfaces here.
   */
  injector.bindImpl<ILogger, SpdlogLogger>();
  injector.bindImpl<IClock, Clock>();

  /**
   * Custom recipes for types should be added here.
   */
  // injector.addRecipe<T>(...)
}
}  // namespace omulator::di
