/**
 * We use this file to isolate injector recipes, and the implied dependencies, to a single
 * translation unit.
 */

#include "omulator/Clock.hpp"
#include "omulator/CoreGraphicsEngine.hpp"
#include "omulator/IGraphicsBackend.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/InputHandler.hpp"
#include "omulator/Interpreter.hpp"
#include "omulator/NullWindow.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/SpdlogLogger.hpp"
#include "omulator/SystemWindow.hpp"
#include "omulator/VulkanBackend.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/MessageQueueFactory.hpp"
#include "omulator/props.hpp"
#include "omulator/util/CLIInput.hpp"
#include "omulator/util/CLIParser.hpp"
#include "omulator/util/TypeHash.hpp"
#include "omulator/vkmisc/Initializer.hpp"
#include "omulator/vkmisc/Pipeline.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <map>
#include <memory_resource>
#include <thread>

using omulator::util::TypeHash;

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
  injector.addCtorRecipe<CoreGraphicsEngine, di::Injector &>();
  injector.addCtorRecipe<VulkanBackend,
                         ILogger &,
                         Injector &,
                         IWindow &,
                         vk::raii::Device &,
                         vkmisc::Swapchain &,
                         vkmisc::Pipeline &,
                         vkmisc::DeviceQueues_t &>();
  injector.addCtorRecipe<util::CLIInput, ILogger &, msg::MailboxRouter &>();
  injector.addCtorRecipe<vkmisc::Swapchain, Injector &, ILogger &, IWindow &, vk::raii::Device &>();
  injector.addCtorRecipe<vkmisc::Pipeline,
                         ILogger &,
                         vk::raii::Device &,
                         vkmisc::Swapchain &,
                         PropertyMap &>();

  vkmisc::install_vk_initializer_rules(injector);

  /**
   * Implementations should be bound to interfaces here.
   */
  injector.bindImpl<IClock, Clock>();
  injector.bindImpl<IGraphicsBackend, VulkanBackend>();

  if(injector.get<PropertyMap>().get_prop<bool>(props::HEADLESS).get()) {
    injector.bindImpl<IWindow, NullWindow>();
  }
  else {
    injector.bindImpl<IWindow, SystemWindow>();
  }

  /**
   * Custom recipes for types should be added here.
   */
  // injector.addRecipe<T>(...)
}

void Injector::installMinimalRules(Injector &injector) {
  // Only need what is necessary to parse the command line before we call installDefaultRules
  injector.bindImpl<ILogger, SpdlogLogger>();
  injector.addCtorRecipe<PropertyMap, ILogger &>();
  injector.addCtorRecipe<util::CLIParser, ILogger &, PropertyMap &>();
}

}  // namespace omulator::di
