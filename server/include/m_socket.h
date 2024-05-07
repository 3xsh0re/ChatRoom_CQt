#ifndef M_SOCKET_H
#define M_SOCKET_H
#include <netinet/tcp.h>
#include <unistd.h>

#include "address.h"
#include "logger.h"
#include "noncopyable.h"

namespace mtd {
class Socket : public noncopyable {
 public:
  explicit Socket(int fd) : fd_(fd) {}

  ~Socket();

  int get_fd() const { return fd_; }

  void BindAddress(const InetAddress &addr);

  void Listen();

  int Accept(InetAddress &peeraddr);

  void ShutdownWrite();

  void SetTcpNoDelay(bool on);

  void SetReuseAddr(bool on);

  void SetReusePort(bool on);

  void SetKeepAlive(bool on);

 private:
  const int fd_;
};
}  // namespace mtd

#endif