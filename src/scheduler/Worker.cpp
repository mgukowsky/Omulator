#include "omulator/scheduler/Worker.hpp"

#include <cassert>
#include <chrono>
#include <optional>
#include <stdexcept>

using namespace std::chrono_literals;

namespace omulator::scheduler {

Worker::Worker(Worker::WorkerGroup_t &workerGroup, std::pmr::memory_resource *memRsrc)
  : startupLatch_(1),
    workerGroup_(workerGroup),
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
    Job_ty currentJob = std::move(peek_job_());
    jobQueue_.pop_front();

    return currentJob;
  }
}

std::thread::id Worker::thread_id() const noexcept { return thread_.get_id(); }

Job_ty &Worker::peek_job_() {
  // If this happens then it's better to throw, since calling front() on an empty std::list
  // causes undefined behavior. This should never happen provided that proper locking of
  // jobQueueLock_ is utilized around calls to this method.
  assert(!jobQueue_.empty());

  return jobQueue_.front();
}

void Worker::steal_job_() {
  Worker * otherWorker     = nullptr;
  Priority highestPriority = Priority::IGNORE;

  for(auto &workerIt : workerGroup_) {
    if(workerIt.get() != this) {
      Worker &worker = *workerIt;

      // We cheat a little bit here since we have access to the other Workers' private variables,
      // but this is necessary to ensure that the following few lines are executed atomically.
      // Otherwise, there is a possibility that Worker::peek_job_() could be invoked on another
      // worker with no jobs in its queue (e.g. the job was moved between the call to num_jobs() and
      // peek_job_()), which would cause undefined behavior.
      std::scoped_lock workerLock(worker.jobQueueLock_);
      if(worker.num_jobs() > 0) {
        Job_ty &job = worker.peek_job_();

        if(job.priority == Priority::MAX) {
          otherWorker = &worker;
          break;
        }
        else if(job.priority > highestPriority) {
          otherWorker     = &worker;
          highestPriority = job.priority;
        }
      }
    }
  }

  // otherWorker will be nullptr if no other Workers have any jobs
  if(otherWorker != nullptr) {
    // N.B. there is a race condition that can occur here where the job that we peeked at earlier
    // could have since been popped out of the other Worker's queue (either by the otherWorker
    // itself or from another thread that stole it first). In this case the results are benign,
    // since the next line would result in the execution of a job with Job_ty::NULL_TASK.
    Job_ty job = otherWorker->pop_job();
    job.task();
  }
}

void Worker::thread_proc_() {
  // Don't do anything until the parent thread is ready.
  startupLatch_.wait();

  /**
   * Since it's possible for the worker to be notified before it enters an alertable state (i.e.
   * calling a wait function on jobCV_), we periodically have the threads wake up to check if
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

    // In this case, the worker has most likely been awoken due to having slept for a greater amount
    // of time than WORKER_WAIT_TIMEOUT as opposed to having been awoken due to a notification from
    // add_job(), so we attempt to steal a job sitting in the queue of another worker.
    if(jobQueue_.empty() && !done_) {
      steal_job_();
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
