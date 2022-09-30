#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MailboxRouter.hpp"

#include <functional>
#include <string_view>
#include <thread>

namespace omulator {

/**
 * Responds to messages in a separate thread.
 */
class Subsystem {
public:

  /**
   * Starts the underlying thread. The thread will wake to service messages send to the mailbox
   * retrieved from mbrouter using mailboxToken.
   *
   * Also allows for two lifecycle hook callbacks: onStart will be invoked when the thread starts
   * prior to the execution of thrd_proc_, and onEnd will be invoked prior to the thread's exit
   * after thrd_proc_ returns. These callbacks are useful for thread-specific init/deinit, such as
   * for thread-specific variables.
   */
  Subsystem(ILogger                  &logger,
            std::string_view          name,
            msg::MailboxRouter       &mbrouter,
            const msg::MailboxToken_t mailboxToken,
            std::function<void()>     onStart = PASS_,
            std::function<void()>     onEnd   = PASS_);

  /**
   * Sends a message to wake the underlying thread, in case it is waiting on a recv() call, and
   * blocks until the thread exits.
   */
  virtual ~Subsystem();

  /**
   * Returns the name of the component.
   */
  std::string_view name() const noexcept;

  /**
   * The function that will be called when messages are sent to the Mailbox. The default
   * implementation is a no-op.
   *
   * While not required, classes which override this method should pass any messages which they do
   * not process to the default implementation by explicitly calling Subsystem::message_proc(msg).
   * This will give the message one last chance at being processed, and ensures that a message which
   * would otherwise be dropped will be appropriately logged.
   */
  virtual void message_proc(const msg::Message &msg);

  /**
   * Request that the underlying thread return exit its message loop and return.
   */
  void stop();

protected:
  ILogger &logger_;

private:
  static constexpr auto PASS_ = [] {};

  void thrd_proc_(std::function<void()> onStart, std::function<void()> onEnd);

  std::string_view name_;

  /**
   * Mailbox which will wake the thread when it receives messages.
   */
  msg::MailboxReceiver receiver_;

  /**
   * Used to send a wakeup message to the receiving Mailbox upon destruction. N.B. that this MUST
   * refer to the same MailboxEndpoint as receiver_
   */
  msg::MailboxSender sender_;

  std::jthread thrd_;
};

}  // namespace omulator
