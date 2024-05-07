#include "m_thread.h"

namespace mtd {
std::atomic_int Thread::num_created_(0);
Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name) {
  SetDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) {
    thread_->detach();
  }
}

void Thread::Start() {
  started_ = true;

  sem_t sem;
  sem_init(&sem, false, 0);

  thread_ = std::make_shared<std::thread>([&]() {
    tid_ = current_thread::Tid();
    sem_post(&sem);
    func_();
  });

  sem_wait(&sem);
}

void Thread::Join() {
  joined_ = true;
  thread_->join();
}

void Thread::SetDefaultName() {
  int num = ++num_created_;
  if (name_.empty()) {
    char buf[32]{};
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = num;
  }
}

}  // namespace mtd
