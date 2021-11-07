#include "omulator/scheduler/Worker.hpp"

#include <chrono>
#include <optional>

using namespace std::chrono_literals;

namespace omulator::scheduler {

Worker::Worker(std::pmr::memory_resource *memRsrc)
  : startupLatch_(1),
    jobQueue_(std::pmr::polymorphic_allocator<Job_ty>(memRsrc)),
    done_(false),
    thread_(&Worker::thread_proc_, this) {
  startupLatch_.count_down();
}

Worker::~Worker() {
  done_ = true;
  jobCV_.notify_one();

  // TODO: put in code to kill the thread if it takes more than a few secs to respond...
  if(thread_.joinable()) {
    thread_.join();
  }
}

std::size_t Worker::num_jobs() const noexcept { return jobQueue_.size(); }

Job_ty Worker::pop_job() {
  std::scoped_lock queueLock(jobQueueLock_);

  if(jobQueue_.empty()) {
    return Job_ty();
  }
  else {
    Job_ty currentJob = std::move(jobQueue_.front());
    jobQueue_.pop_front();

    return currentJob;
  }
}

std::thread::id Worker::thread_id() const noexcept { return thread_.get_id(); }

void Worker::thread_proc_() {
  // Don't do anything until the parent thread is ready.
  startupLatch_.wait();

  /**
   * Since it's possible for the worker to be notified before it enters an alertable state (i.e.
   * calling a wait functiuon on jobCV_), we periodically have the threads wake up to check if
   * there is any work to be done. The quantity here is arbitrary, and can be tuned as per
   * profiling results if necessary.
   *
   * N.B. putting this here instead of at file scope makes the static analyzer happy, since
   * operator""ms can technically throw.
   */
  constexpr auto WORKER_WAIT_TIMEOUT = 10ms;

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
      jobCV_.wait_for(
        cvLock, WORKER_WAIT_TIMEOUT, [this]() noexcept { return !jobQueue_.empty() || done_; });
    }

    // This empty check does not need to be protected; even if a task is added while the check is
    // taking place, if will be caught by the wait_for predicate.
    while(!jobQueue_.empty()) {
      if(done_) {
        break;
      }

      Job_ty currentJob = pop_job();
      if(currentJob.priority != Priority::IGNORE) {
        currentJob.task();
      }
      // TODO: When the current task finishes executing, check to see if an exception was raised
      // and, if so, log it.
    }
  }
}

} /* namespace omulator::scheduler */
