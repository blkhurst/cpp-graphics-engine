#include <blkhurst/assets/thread_dispatcher.hpp>

#include <future>
#include <spdlog/spdlog.h>

namespace blkhurst {

ThreadDispatcher::ThreadDispatcher(int numWorkers)
    : mainThreadId_(std::this_thread::get_id()) {

  if (numWorkers <= 0) {
    const unsigned availableThreads = std::thread::hardware_concurrency();
    numWorkers = (availableThreads > 1) ? int(availableThreads) - 1 : 1;
  }

  // Start Worker Threads
  workers_.reserve(numWorkers);
  for (int i = 0; i < numWorkers; ++i) {
    workers_.emplace_back([this, i] {
      spdlog::debug("ThreadDispatcher started workerLoop#{}", i);
      workerLoop(i);
    });
  }

  spdlog::debug("ThreadDispatcher(workers={}) constructed", numWorkers);
}

ThreadDispatcher::~ThreadDispatcher() {
  requestStop_ = true;

  // Flush & Drain Main Queue
  flushMainQueue();
  {
    std::lock_guard<std::mutex> lock(mainMutex_);
    while (!mainQueue_.empty()) {
      mainQueue_.pop();
    }
  }

  // Drain Worker Queue
  {
    std::lock_guard<std::mutex> lock(workersMutex_);
    while (!workerQueue_.empty()) {
      workerQueue_.pop();
    }
  }

  // Notify All Workers To Wake
  workersCondition_.notify_all();

  // Join Workers
  spdlog::debug("ThreadDispatcher joining {} workers", workers_.size());
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  spdlog::debug("ThreadDispatcher joined all workers");
  spdlog::debug("ThreadDispatcher destructed");
}

bool ThreadDispatcher::enqueueAsync(std::function<void()> func, std::string label) {
  auto token = std::make_shared<CancelToken>();
  {
    std::lock_guard<std::mutex> lock(workersMutex_);

    // If Stopping, Reject New Jobs
    if (requestStop_) {
      spdlog::trace("ThreadDispatcher::enqueueAsync called after stop requested; dropping job");
      return false;
    }

    Job job;
    job.label = std::move(label);
    job.run = std::move(func);
    job.cancelToken = token;
    workerQueue_.push(std::move(job));
  }
  workersCondition_.notify_one();
  return true;
}

bool ThreadDispatcher::enqueueMain(std::function<void()> func, std::string label) {
  auto token = std::make_shared<CancelToken>();

  // If Stopping, Reject New Jobs
  if (requestStop_) {
    spdlog::trace("ThreadDispatcher::enqueueMain called after stop requested; dropping job");
    return false;
  }

  // If on MainThread, Run Immediately
  if (isMainThread()) {
    func();
    return true;
  }

  // Otherwise, Enqueue the Job
  {
    std::lock_guard<std::mutex> lock(mainMutex_);
    Job job;
    job.label = std::move(label);
    job.run = std::move(func);
    job.cancelToken = token;
    mainQueue_.push(std::move(job));
  }
  return true;
}

void ThreadDispatcher::flushMainQueue() {
  if (!isMainThread()) {
    return;
  }

  // Use localQueue To Allow Unlocking mainMutex_ For Job Execution
  std::queue<Job> localQueue;
  {
    std::lock_guard<std::mutex> lock(mainMutex_);
    std::swap(localQueue, mainQueue_);
  }

  while (!localQueue.empty()) {
    // Get Next Job & Remove from Queue
    Job job = std::move(localQueue.front());
    localQueue.pop();

    // Check Cancelled
    if (job.cancelToken && job.cancelToken->isCancelled()) {
      continue;
    }

    // Run Job with Active Worker Count (Unaffected by Cancels) (Updated to use RAII)
    {
      // MainThread activeWorkerLabel may not appear since added/removed immediately (sync)
      JobScope scope(this, -1, job.label); // -1 for MainThread
      job.run();
    }
  }
}

//! REFACTOR
void ThreadDispatcher::invokeMainAndWait(std::function<void()> func) {
  // If Stopping, Reject New Jobs
  if (requestStop_) {
    spdlog::trace("ThreadDispatcher::invokeMainAndWait called after stop requested; dropping job");
    return;
  }

  // If on MainThread, Run Immediately
  if (isMainThread()) {
    func();
    return;
  }

  // Copyable Job via Promise/Future
  auto promise = std::make_shared<std::promise<void>>();
  std::future<void> fut = promise->get_future();

  // Enqueue the Job
  {
    std::lock_guard<std::mutex> lock(mainMutex_);

    Job job;
    job.label = "invokeMainAndWait"; // TODO ADD
    job.run = [func = std::move(func), promise]() mutable {
      func();
      promise->set_value(); // Signal Completion
    };
    // Optional CancelToken
    // mainQueue Polled, no condition variable needed
    mainQueue_.push(std::move(job));
  }

  // Wait Until MainThread Runs flushMainQueue() with Timeout
  using namespace std::chrono_literals; // Used for 2ms
  const auto timeout = std::chrono::steady_clock::now() + 3s;
  for (;;) {
    if (requestStop_) {
      spdlog::debug("ThreadDispatcher::invokeMainAndWait exiting due to shutdown request");
      break;
    }
    if (fut.wait_for(2ms) == std::future_status::ready) {
      break;
    }
    if (std::chrono::steady_clock::now() >= timeout) {
      spdlog::warn("ThreadDispatcher::invokeMainAndWait timed out; exiting without completion");
      break;
    }
  }
}

bool ThreadDispatcher::isIdle() {
  // No queued background jobs, no queued main-thread jobs, and no active workers
  std::lock_guard<std::mutex> workersLock(workersMutex_);
  std::lock_guard<std::mutex> mainLock(mainMutex_);
  return workerQueue_.empty() && mainQueue_.empty() &&
         (activeWorkers_.load(std::memory_order_relaxed) == 0);
}

bool ThreadDispatcher::hasActiveWorkers() {
  return activeWorkers_.load(std::memory_order_relaxed) != 0;
}

void ThreadDispatcher::cancelPendingJobs() {
  // Cancel & Drain mainQueue
  {
    std::lock_guard<std::mutex> lock(mainMutex_);
    while (!mainQueue_.empty()) {
      auto& job = mainQueue_.front();
      if (job.cancelToken) {
        job.cancelToken->cancel();
        spdlog::trace("ThreadDispatcher: cancelling main job '{}'", job.label);
      }
      mainQueue_.pop();
    }
  }
  // Cancel & Drain workerQueue
  {
    std::lock_guard<std::mutex> lock(workersMutex_);
    while (!workerQueue_.empty()) {
      auto& job = workerQueue_.front();
      if (job.cancelToken) {
        job.cancelToken->cancel();
        spdlog::trace("ThreadDispatcher: cancelling async job '{}'", job.label);
      }
      workerQueue_.pop();
    }
  }
}

void ThreadDispatcher::workerLoop(int workerIndex) {
  for (;;) {
    Job job;

    {
      // Wait for Job or Stop Request
      std::unique_lock<std::mutex> lock(workersMutex_);
      workersCondition_.wait(lock, [this] { return requestStop_ || !workerQueue_.empty(); });

      // Check for Stop Request; Drop Remaining Jobs
      if (requestStop_) {
        return;
      }

      // Get Next Job & Remove from Queue
      job = std::move(workerQueue_.front());
      workerQueue_.pop();
    }

    // Check Cancelled
    if (job.cancelToken && job.cancelToken->isCancelled()) {
      continue;
    }

    // Run Job with Active Worker Count (Unaffected by Cancels) (Updated to use RAII)
    {
      JobScope scope(this, workerIndex, job.label);
      job.run();
    }
  }
}

bool ThreadDispatcher::isMainThread() const {
  return std::this_thread::get_id() == mainThreadId_;
}

void ThreadDispatcher::addThreadLabel_(int jobId, const std::string& label) {
  std::lock_guard<std::mutex> lock(threadLabelsMutex_);
  threadLabels_.push_back({static_cast<int>(jobId), label});
}

void ThreadDispatcher::removeThreadLabel_(int jobId) {
  std::lock_guard<std::mutex> lock(threadLabelsMutex_);
  auto found = std::find_if(threadLabels_.begin(), threadLabels_.end(),
                            [jobId](const ThreadLabel& job) { return job.id == jobId; });
  if (found != threadLabels_.end()) {
    threadLabels_.erase(found);
  }
}

std::string ThreadDispatcher::activeWorkerLabel() {
  std::lock_guard<std::mutex> lock(threadLabelsMutex_);
  if (!threadLabels_.empty()) {
    return threadLabels_.front().label; // Oldest Active Job
  }
  return "No Active Jobs";
}

} // namespace blkhurst
