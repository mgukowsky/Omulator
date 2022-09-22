#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MailboxReceiver.hpp"
#include "omulator/msg/MailboxSender.hpp"

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
   * corresponding to the receiver argument. N.B. that the receiver and sender MUST refer to the
   * same mailbox, otherwise the thread may not exit (see the send() call in the destructor).
   */
  Subsystem(ILogger             &logger,
            std::string_view     name,
            msg::MailboxReceiver receiver,
            msg::MailboxSender   sender);

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
  void thrd_proc_();

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
