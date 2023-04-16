#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MailboxRouter.hpp"

#include <atomic>
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
   *
   * N.B. that ALL derived classes should call start() at some point after construction in order to
   * begin execution of the underlying thread and receive messages! This is necessary in order to
   * ensure that the underlying thread (which will start before the child class is fully
   * constructed) doesn't start running and receiving messages until the derived class is fully
   * constructed (e.g. all necessary on*() calls have been made in the constructor)! If the
   * Subsystems are created using System::make_subsystem_list, then start() will be invoked once the
   * Subsystem is fully constructed.
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
   * Begin execution of the underlying thread. Has no effect if called more than once.
   */
  void start();

  /**
   * Request that the underlying thread return exit its message loop and return.
   */
  void stop();

protected:

  ILogger &logger_;

  /**
   * Mailbox which will wake the thread when it receives messages.
   */
  msg::MailboxReceiver receiver_;

private:
  static constexpr auto PASS_ = [] {};

  void thrd_proc_(std::function<void()> onStart, std::function<void()> onEnd);

  std::string_view name_;

  /**
   * Used to send a wakeup message to the receiving Mailbox upon destruction. N.B. that this MUST
   * refer to the same MailboxEndpoint as receiver_
   */
  msg::MailboxSender sender_;

  std::atomic_bool startSignal_;
  std::jthread     thrd_;
};

}  // namespace omulator
