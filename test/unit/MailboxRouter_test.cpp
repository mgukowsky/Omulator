#include "omulator/msg/MailboxRouter.hpp"

#include "omulator/di/Injector.hpp"
#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/Mailbox.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/msg/Package.hpp"
#include "omulator/util/ObjectPool.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"

#include <gtest/gtest.h>

#include <memory>

using ::testing::Exactly;

using omulator::di::Injector;
using omulator::di::TypeHash;
using omulator::msg::Mailbox;
using omulator::msg::MailboxRouter;
using omulator::msg::MessageBuffer;
using omulator::msg::Package;
using omulator::util::ObjectPool;

namespace {
std::unique_ptr<LoggerMock> pLogger;
std::unique_ptr<Injector>   pInjector;

}  // namespace

class MailboxRouter_test : public ::testing::Test {
protected:
  void SetUp() override {
    pLogger.reset(new LoggerMock);
    pInjector.reset(new Injector);

    pInjector->addRecipe<ObjectPool<MessageBuffer>>(
      [](Injector &inj) { return inj.containerize(new ObjectPool<MessageBuffer>(0x10)); });
    pInjector->addRecipe<ObjectPool<Package>>(
      [](Injector &inj) { return inj.containerize(new ObjectPool<Package>(0x10)); });
  }

  void TearDown() override {
    // We _HAVE_ to delete the logger mock here, as mock objects need to be destroyed
    // after tests end and before GoogleMock begins its global teardown, otherwise we
    // will get an exception thrown. See https://github.com/google/googletest/issues/1963
    pLogger.reset();
    pInjector.reset();
  }
};

TEST_F(MailboxRouter_test, claimAndThenGet) {
  MailboxRouter mailboxRouter(*pLogger, *pInjector);

  Mailbox &mailboxA = mailboxRouter.claim_mailbox(TypeHash<int>);
  Mailbox &mailboxB = mailboxRouter.get_mailbox(TypeHash<int>);
  Mailbox &mailboxC = mailboxRouter.get_mailbox(TypeHash<int>);

  EXPECT_EQ(&mailboxA, &mailboxB)
    << "MailboxRouter::claimMailbox() and MailboxRouter::getMailbox(), when invoked with identical "
       "arguments, should return the same Mailbox instance";
  EXPECT_EQ(&mailboxA, &mailboxC)
    << "MailboxRouter::claimMailbox() and MailboxRouter::getMailbox(), when invoked with identical "
       "arguments, should return the same Mailbox instance";

  EXPECT_EQ(&mailboxB, &mailboxC)
    << "Two separate calls to MailboxRouter::getMailbox(), invoked with the same arguments, should "
       "return the same Mailbox instance";
}

TEST_F(MailboxRouter_test, doubleClaim) {
  MailboxRouter mailboxRouter(*pLogger, *pInjector);

  [[maybe_unused]] Mailbox &mailboxA = mailboxRouter.claim_mailbox(TypeHash<int>);

  EXPECT_CALL(*pLogger, error).Times(Exactly(1));
  [[maybe_unused]] Mailbox &mailboxB = mailboxRouter.claim_mailbox(TypeHash<int>);

  // We have to manually destruct the mock object here in order to validate the EXPECT_CALL
  // expectation above, otherwise we would get a spurious GMock failure because pLogger would be
  // destroyed outside the scope where the EXPECT_CALL invocation was made, therefore making it
  // impossible to verify the mock calls.
  pLogger.reset();
}
