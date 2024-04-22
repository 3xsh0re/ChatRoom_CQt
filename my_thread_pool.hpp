#ifndef MY_THREADPOOL_H
#define MY_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <iostream>
#include <shared_mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mtd {

  class ThreadPool {
  public:
    ThreadPool(int max_thread_size);
    ~ThreadPool();

    void AddTask(std::function<void()>&& fn);

    template <typename Func, typename... Args>
    void AddTask(Func&& f, Args &&...args) {
      std::function<void()> task =
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
      {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        tasks_.emplace(std::move(task));
        cv_.notify_one();
      }
    }

    int GetIdleSize();

  private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mtx_;
    std::shared_mutex idle_size_mtx_;
    std::condition_variable cv_;
    bool is_stop_;
    int idle_size_;
  };

}  // namespace mtd

#endif  // THREADPOOL_H
