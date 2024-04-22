#include "my_thread_pool.hpp"

namespace mtd {

  ThreadPool::ThreadPool(int max_thread_size)
    : is_stop_(false), idle_size_(max_thread_size) {
    for (int i = 0; i < max_thread_size; ++i) {
      threads_.emplace_back([this]() -> void {
        while (1) {
          std::unique_lock<std::mutex> lock(queue_mtx_);
          {
            std::unique_lock<std::shared_mutex> size_lock(idle_size_mtx_);
            --idle_size_;
          }
          cv_.wait(lock,
            [this]() -> bool { return !tasks_.empty() || is_stop_; });

          if (is_stop_ && tasks_.empty()) return;

          std::function<void()> task = std::move(tasks_.front());
          tasks_.pop();

          lock.unlock();//必须解锁

          task();
          {
            std::unique_lock<std::shared_mutex> size_lock(idle_size_mtx_);
            ++idle_size_;
          }
        }
        });
    }
  }

  void ThreadPool::AddTask(std::function<void()>&& fn) {
    {
      std::lock_guard<std::mutex> lock(queue_mtx_);
      tasks_.emplace(std::move(fn));
      cv_.notify_one();
    }     
  }

  ThreadPool::~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(queue_mtx_);
      is_stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : threads_) {
      t.join();
    }
  }

  int ThreadPool::GetIdleSize() {
    {
      std::shared_lock<std::shared_mutex> slock(idle_size_mtx_);
      return idle_size_;
    }
  }

}  // namespace mtd
