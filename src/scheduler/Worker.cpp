#include "omulator/scheduler/Worker.hpp"

#include <chrono>
#include <optional>

using namespace std::chrono_literals;

namespace {

/**
 * Since it's possible for the worker to be notified before it enters an alertable state (i.e.
 * calling a wait functiuon on jobCV_), we periodically have the threads wake up to check if
 * there is any work to be done. The quantity here is arbitrary, and can be tuned as per
 * profiling results if necessary.
 */
const auto WORKER_WAIT_TIMEOUT = 10ms;

}  // namespace

namespace omulator::scheduler {

Worker::Worker() : startupLatch_(1), done_(false), thread_(&Worker::thread_proc_, this) {
  // As long as the job queue is initially empty, the following line is pointless. If the
  // initial job queue is NOT empty, however, uncomment the following line.
  // std::make_heap(jobQueue_.begin(), jobQueue_.end());
  startupLatch_.count_down();

  // TODO: Do we want to do anything with thread priority? Do we care? Need to profile tho...
}

Worker::~Worker() {
  done_ = true;
  jobCV_.notify_one();

  // TODO: put in code to kill the thread if it takes more than a few secs to respond...
  if(thread_.joinable()) {
    thread_.join();
  }
}

const std::deque<Job_ty> &Worker::job_queue() const noexcept { return jobQueue_; }

std::thread::id Worker::thread_id() const noexcept { return thread_.get_id(); }

void Worker::thread_proc_() {
  // Don't do anything until the parent thread is ready.
  startupLatch_.wait();

  while(!done_) {
    {
      std::unique_lock cvLock(jobQueueLock_);

      // N.B. we use wait_for in case there is some subtle race condition that can arise on the
      // implementation's end.
      //
      // If nothing else it helps me sleep easier :)
      //
      // The period chosen here doesn't represent anything special; if can certainly be changed
      // per profiler results if necessary.
      jobCV_.wait_for(cvLock, WORKER_WAIT_TIMEOUT, [this]() noexcept { return !jobQueue_.empty() || done_; });
    }

    // This empty check does not need to be protected; even if a task is added while the check is
    // taking place, if will be caught by the wait_for predicate.
    while(!jobQueue_.empty()) {
      if(done_) {
        break;
      }

      Job_ty currentJob;
      {
        std::scoped_lock queueLock(jobQueueLock_);

        // We can't simply grab a reference to the head and pop it later, as other threads
        // may push additional work at a higher priority while this task is executing, thereby
        // invalidating iterators (i.e. pop_heap and pop_back below might operate on a higher
        // priority task that was pushed while the currentJob was executing).
        //
        // To avoid this, we move the packaged_task into a new object and then immediately
        // update the state of the queue by destroying the moved-from object.
        //
        // TODO: would this be cleaner by wrapping the packaged_task in a smart pointer? At
        // a minimum this would minimize the amount of work needed to be done by std::move...
        currentJob = std::move(jobQueue_.front());
        std::pop_heap(jobQueue_.begin(), jobQueue_.end(), JOB_COMPARATOR);
        jobQueue_.pop_back();
      }

      currentJob.task();
      // TODO: When the current task finishes executing, check to see if an exception was raised
      // and, if so, log it.
    }
  }
}

} /* namespace omulator::scheduler */