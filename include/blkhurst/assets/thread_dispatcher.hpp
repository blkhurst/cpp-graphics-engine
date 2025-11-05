#pragma once
#include <condition_variable>
#include <functional>
#include <queue>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

namespace blkhurst {

struct CancelToken {
  std::atomic<bool> cancelled{false};
  void cancel() {
    cancelled.store(true);
  }
  [[nodiscard]] bool isCancelled() const {
    return cancelled.load();
  }
};

class ThreadDispatcher {
public:
  ThreadDispatcher(int numWorkers = 0);
  ~ThreadDispatcher();

  ThreadDispatcher(const ThreadDispatcher&) = delete;
  ThreadDispatcher(ThreadDispatcher&&) = delete;
  ThreadDispatcher& operator=(const ThreadDispatcher&) = delete;
  ThreadDispatcher& operator=(ThreadDispatcher&&) = delete;

  bool enqueueAsync(std::function<void()> func, std::string label = "");
  bool enqueueMain(std::function<void()> func, std::string label = "");
  void flushMainQueue();

  // Temporary - since "Texture" contains GL calls, need to invoke on main thread
  void invokeMainAndWait(std::function<void()> func);

  [[nodiscard]] bool isIdle();
  [[nodiscard]] bool hasActiveWorkers();
  [[nodiscard]] std::string activeWorkerLabel();

  void cancelPendingJobs();

private:
  std::thread::id mainThreadId_;

  struct Job {
    std::string label;
    std::function<void()> run;
    std::shared_ptr<CancelToken> cancelToken;
  };

  // Background Workers
  std::vector<std::thread> workers_;
  //
  std::mutex workersMutex_; // Protects workerQueue_
  std::queue<Job> workerQueue_;
  std::condition_variable workersCondition_;
  void workerLoop(int workerIndex);

  // Main-Thread Queue
  std::mutex mainMutex_;
  std::queue<Job> mainQueue_;

  // Shutdown
  std::atomic<bool> requestStop_ = false;

  // Activity counters
  friend class JobScope;
  std::atomic<uint64_t> activeWorkers_{0};

  // Utilities
  [[nodiscard]] bool isMainThread() const;

  // Thread Labels
  struct ThreadLabel {
    int id;
    std::string label;
  };
  std::mutex threadLabelsMutex_;
  std::vector<ThreadLabel> threadLabels_;
  void addThreadLabel_(int jobId, const std::string& label);
  void removeThreadLabel_(int jobId);
};

// RAII Scope To Manage Active Worker Count And Thread Labels
class JobScope {
public:
  JobScope(ThreadDispatcher* dispatcher, int threadId, const std::string& label)
      : dispatcher_(dispatcher),
        id_(threadId) {
    dispatcher_->activeWorkers_.fetch_add(1, std::memory_order_relaxed);
    dispatcher_->addThreadLabel_(id_, label);
  }

  ~JobScope() {
    dispatcher_->removeThreadLabel_(id_);
    dispatcher_->activeWorkers_.fetch_sub(1, std::memory_order_relaxed);
  }

  JobScope(const JobScope&) = default;
  JobScope(JobScope&&) = delete;
  JobScope& operator=(const JobScope&) = default;
  JobScope& operator=(JobScope&&) = delete;

private:
  ThreadDispatcher* dispatcher_;
  int id_;
};

} // namespace blkhurst
