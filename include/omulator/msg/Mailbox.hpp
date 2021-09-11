#pragma once

#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/util/ObjectPool.hpp"

namespace omulator::msg {

class Mailbox {
public:
  using Pool_t = util::ObjectPool<MessageBuffer>;

  Mailbox(Pool_t &pool) : pool_(pool) { }

private:
  Pool_t &pool_;
};

}  // namespace omulator::msg
