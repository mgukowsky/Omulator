#include "omulator/PrimitiveIO.hpp"
#include "omulator/oml_types.hpp"

#include <atomic>
#include <cstdlib>
#include <latch>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace omulator::test {

/**
 * A utitily class used to synchronize multi-step operations which take place across multiple
 * threads.
 *
 * A common design pattern within small applications such as demos and tests is to synchonize a
 * given flow of events across multiple threads using a std::promise<void>. This idiom makes it easy
 * to send one-shot notifications across threads to enforce a specific flow of events. In the
 * event that more than one of these notifcations are used (e.g. in the event that multiple blocking
 * synchonization points are needed to ensure deterministic execution), then the multiple
 * std::promise<void> objects will often be annotated with a number to indicate the order in which
 * they will execute.
 *
 * Here is an example of this idiom:
 *
 * #include <iostream>
 * #include <future>
 * #include <thread>
 *
 * int main() {
 *     std::promise<void> signal1, signal2, signal3, doneSignal;
 *
 *     std::jthread t3([&]{
 *         signal3.get_future().wait();
 *         std::cout << "thread 3\n";
 *         doneSignal.set_value();
 *     });
 *
 *     std::jthread t2([&]{
 *         signal2.get_future().wait();
 *         std::cout << "thread 2\n";
 *         signal3.set_value();
 *     });
 *
 *     std::jthread t1([&]{
 *         signal1.get_future().wait();
 *         std::cout << "thread 1\n";
 *         signal2.set_value();
 *     });
 *
 *     std::cout << "ready\n";
 *     signal1.set_value();
 *     doneSignal.get_future().wait();
 *     std::cout << "done\n";
 *
 *     return 0;
 * }
 *
 * // Prints:
 * // ready
 * // thread 1
 * // thread 2
 * // thread 3
 * // done
 *
 *
 * This class allows the previous example to be rewritten as follows:
 *
 *  Sequencer sequencer(4);

 *  std::jthread t1([&] {
 *    sequencer.wait_for_step(1);
 *    std::cout << "thread 1\n";
 *    sequencer.advance_step(2);
 *  });

 *  std::jthread t2([&] {
 *    sequencer.wait_for_step(2);
 *    std::cout << "thread 2\n";
 *    sequencer.advance_step(3);
 *  });

 *  std::jthread t3([&] {
 *    sequencer.wait_for_step(3);
 *    std::cout << "thread 3\n";
 *    sequencer.advance_step(4);
 *  });
 *
 *  std::cout << "ready\n";
 *  sequencer.advance_step(1);
 *  sequencer.wait_for_step(4);
 *  std::cout << "done\n";
 *
 * A downside of the design of this class is that its interface my force client code to be written
 * in a somewhat brittle manner. For example, the addition or removal of a step will force all steps
 * to be updated, the benefit of having sequence points numbered explicitly in the source code
 * outweighs this, especially in light of the likelihood that apps which make use of this class will
 * likely never have more than a few steps.
 *
 * A generator incrementor could also be used (something like Go's iota utility comes to mind) to
 * specify sequence points and make client code less brittle, however the benefits of having the
 * sequence steps explicitly numbered is likely to outweight this benefit.
 */
class Sequencer {
public:
  Sequencer(const U32 totalSteps, bool verbose = false)
    // The +1 is necessary since totalSteps indicates the last step that will be advanced to and
    // (potentially) waited on, therefore we need signals_ to have an actual promise object at that
    // index.
    : step_(0), totalSteps_(totalSteps), verbose_(verbose) {
    for(U32 i = 0; i < (totalSteps + 1); ++i) {
      signals_.emplace_back(std::make_unique<std::latch>(1));
    }
  }

  /**
   * Forces abnormal termination of the program if the step count is not equal to the total number
   * of expected steps.
   */
  ~Sequencer() {
    if(step_ != totalSteps_) {
      std::stringstream ss;
      ss << "Sequencer expected " << totalSteps_
         << " at time of destruction, but the current step count is " << step_ << std::endl;
      PrimitiveIO::log_err(ss.str().c_str());

      // Call abort directly since we're in a destructor
      std::abort();
    }
  }

  /**
   * Increment the current step by 1, and notifies any thread waiting for this to be the new step
   * count.
   */
  void advance_step(const U32 nextStep) {
    std::scoped_lock lck(mtx_);

    assert_legal_step_(nextStep);

    if(step_ != nextStep - 1) {
      std::stringstream ss;
      ss << "advance_step attempted to advance to step " << nextStep << ", but the current step is "
         << step_ << std::endl;
      throw std::runtime_error(ss.str());
    }

    if(verbose_) {
      std::cout << "Sequencer: advancing to step " << nextStep << std::endl;
    }
    ++step_;
    signals_.at(nextStep)->count_down();
  }

  U32 current_step() const noexcept {
    std::scoped_lock lck(mtx_);
    return step_;
  }

  /**
   * Blocks the thread until the step count advances to the desired step.
   */
  void wait_for_step(const U32 waitStep) {
    {
      std::scoped_lock lck(mtx_);
      assert_legal_step_(waitStep);
      if(step_ > waitStep) {
        std::stringstream ss;
        ss << "wait_for_step tried to wait for step " << waitStep << ", but the current step is "
           << step_ << std::endl;
        throw std::runtime_error(ss.str());
      }
    }

    if(verbose_) {
      std::cout << "Sequencer: waiting for step " << waitStep << std::endl;
    }
    signals_.at(waitStep)->wait();
  }

private:
  void assert_legal_step_(const U32 step) {
    if(step > totalSteps_) {
      std::stringstream ss;
      ss << "Invalid step '" << step << "'; expected total step count is " << totalSteps_
         << std::endl;
      throw std::runtime_error(ss.str());
    }
  }

  mutable std::mutex mtx_;

  std::vector<std::unique_ptr<std::latch>> signals_;

  std::atomic<U32> step_;
  const U32        totalSteps_;
  bool             verbose_;
};

}  // namespace omulator::test
