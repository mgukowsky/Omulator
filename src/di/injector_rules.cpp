/**
 * We use this file to isolate injector recipes, and the implied dependencies, to a single
 * translation unit.
 */

#include "omulator/Clock.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/SpdlogLogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/di/TypeHash.hpp"
#include "omulator/scheduler/Scheduler.hpp"

#include <map>
#include <memory_resource>
#include <thread>

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
  injector.addCtorRecipe<msg::MailboxRouter, ILogger &, Injector &>();

  /**
   * Implementations should be bound to interfaces here.
   */
  injector.bindImpl<ILogger, SpdlogLogger>();
  injector.bindImpl<IClock, Clock>();

  /**
   * Finally, install any custom recipes provided above.
   */
  injector.addRecipes(recipeMap);

  injector.addRecipe<util::ObjectPool<msg::MessageBuffer>>([](Injector &inj) {
    return inj.containerize(new util::ObjectPool<msg::MessageBuffer>(0x100));
  });

  injector.addRecipe<util::ObjectPool<msg::Package>>(
    [](Injector &inj) { return inj.containerize(new util::ObjectPool<msg::Package>(0x100)); });

  injector.addRecipe<scheduler::Scheduler>([](Injector &inj) {
    return inj.containerize(new scheduler::Scheduler(std::thread::hardware_concurrency(),
                                                     inj.get<IClock>(),
                                                     std::pmr::get_default_resource(),
                                                     inj.get<msg::MailboxRouter>(),
                                                     inj.get<ILogger>()));
  });
}
}  // namespace omulator::di
