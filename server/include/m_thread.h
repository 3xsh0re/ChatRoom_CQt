#ifndef M_THREAD_H
#define M_THREAD_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <semaphore.h>

#include "current_thread.h"
#include "logger.h"
#include "noncopyable.h"

namespace mtd {
class Thread : public noncopyable {
 public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc func, const std::string& name = std::string());

  ~Thread();

  void Start();

  void Join();

  bool get_started() const { return started_; }

  pid_t get_tid() const { return tid_; }

  const std::string& get_name() const { return name_; }

  void SetDefaultName();

  static int get_num_created() { return num_created_; }

 private:
  bool started_;
  bool joined_;
  std::shared_ptr<std::thread> thread_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  static std::atomic_int num_created_;
};
}  // namespace mtd

#endif