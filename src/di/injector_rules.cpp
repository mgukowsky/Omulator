/**
 * We use this file to isolate injector recipes, and the implied dependencies, to a single
 * translation unit.
 */

#include "omulator/ILogger.hpp"
#include "omulator/SpdlogLogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/di/TypeHash.hpp"

#include <map>

using omulator::di::TypeHash;

namespace {
/**
 * Custom recipes for types should be added here.
 */
omulator::di::Injector::RecipeMap_t recipeMap{

};
}  // namespace

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

  /**
   * Finally, install any custom recipes provided above.
   */
  injector.addRecipes(recipeMap);
}
}  // namespace omulator::di
