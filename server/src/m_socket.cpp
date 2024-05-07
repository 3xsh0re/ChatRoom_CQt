#include "m_socket.h"

namespace mtd {
Socket::~Socket() { ::close(fd_); }

void Socket::BindAddress(const InetAddress &addr) {
  if (0 != ::bind(fd_, addr.GetSockAddr(), sizeof(sockaddr_in))) {
    LOG_FATAL("func:%s, bind sockfd:%d fail", __FUNCTION__, fd_);
  }
}

void Socket::Listen() {
  if (0 != ::listen(fd_, 1024)) {
    LOG_FATAL("func:%s, listen sockfd:%d fail", __FUNCTION__, fd_);
  }
}

int Socket::Accept(InetAddress &peeraddr) {
  sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  int connect_fd =
      ::accept4(fd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connect_fd >= 0) {
    peeraddr.SetAddress(addr);
  } else {
    LOG_ERROR("func:%s, socket, accept %d error, nom = %d", __FUNCTION__, fd_,
              errno);
  }
  return connect_fd;
}

void Socket::ShutdownWrite() {
  if (::shutdown(fd_, SHUT_RD) < 0) {
    LOG_ERROR("func:%s, socket, shutdown error %d", __FUNCTION__, fd_);
  }
}

void Socket::SetTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("func:%s, socket, set nodelay error %d", __FUNCTION__, fd_);
  }
}

void Socket::SetReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("func:%s, socket, set reuse addr error %d", __FUNCTION__, fd_);
  }
}

void Socket::SetReusePort(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("func:%s, socket, set reuse port error %d", __FUNCTION__, fd_);
  }
}

void Socket::SetKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("func:%s, socket, set keep alive error %d", __FUNCTION__, fd_);
  }
}
}  // namespace mtd
