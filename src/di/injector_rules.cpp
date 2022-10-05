/**
 * We use this file to isolate injector recipes, and the implied dependencies, to a single
 * translation unit.
 */

#include "omulator/Clock.hpp"
#include "omulator/IGraphicsBackend.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/InputHandler.hpp"
#include "omulator/Interpreter.hpp"
#include "omulator/SpdlogLogger.hpp"
#include "omulator/SystemWindow.hpp"
#include "omulator/VulkanBackend.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/MessageQueueFactory.hpp"
#include "omulator/util/CLIInput.hpp"
#include "omulator/util/CLIParser.hpp"

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
  injector.addCtorRecipe<msg::MessageQueueFactory, ILogger &>();
  injector.addCtorRecipe<msg::MailboxRouter, ILogger &, msg::MessageQueueFactory &>();
  injector.addCtorRecipe<SystemWindow, ILogger &, InputHandler &>();
  injector.addCtorRecipe<InputHandler, msg::MailboxRouter &>();
  injector.addCtorRecipe<Interpreter, di::Injector &>();
  injector.addCtorRecipe<VulkanBackend, ILogger &>();
  injector.addCtorRecipe<util::CLIInput, ILogger &, msg::MailboxRouter &>();
  injector.addCtorRecipe<util::CLIParser, ILogger &>();

  /**
   * Implementations should be bound to interfaces here.
   */
  injector.bindImpl<ILogger, SpdlogLogger>();
  injector.bindImpl<IClock, Clock>();
  injector.bindImpl<IGraphicsBackend, VulkanBackend>();
  injector.bindImpl<IWindow, SystemWindow>();

  /**
   * Custom recipes for types should be added here.
   */
  // injector.addRecipe<T>(...)
}
}  // namespace omulator::di
