#include "current_thread.h"

namespace mtd {
namespace current_thread {
  __thread int t_cache_tid = 0;

  void CacheTid() {
    if (t_cache_tid == 0) {
      t_cache_tid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
  }
}  // namespace current_thread
  
} // namespace mt   d
