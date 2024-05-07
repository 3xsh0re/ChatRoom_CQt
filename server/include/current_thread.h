#ifndef CURRETN_THREAD_H
#define CURRETN_THREAD_H
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
namespace mtd {
namespace current_thread {
extern __thread int t_cache_tid;
void CacheTid();
inline int Tid() {
  if (__builtin_expect(t_cache_tid == 0, 0)) {
    CacheTid();
  }
  return t_cache_tid;
}
}  // namespace current_thread

}  // namespace mtd

#endif