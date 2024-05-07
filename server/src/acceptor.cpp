#include "acceptor.h"
namespace mtd {

static int CreateNonBlocking() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_FATAL("func:%s, errno:%d", __FUNCTION__, errno);
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr,
                   bool reuse_port)
    : loop_(loop),
      sock_(CreateNonBlocking()),
      accept_channel_(loop, sock_.get_fd()),
      listenning_(false) {
  sock_.SetReuseAddr(true);
  sock_.SetReusePort(true);
  sock_.BindAddress(listen_addr);
  accept_channel_.SetReadCallBack(std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor() {
  accept_channel_.DisableAll();
  accept_channel_.Remove();
}

void Acceptor::Listen() {
  listenning_ = true;
  sock_.Listen();
  accept_channel_.EnableReading();
}

void Acceptor::HandleRead() {
  InetAddress peeraddr;
  int connection_fd = sock_.Accept(peeraddr);
  if (connection_fd >= 0) {
    if (new_cb_) {
      new_cb_(connection_fd, peeraddr);
    } else {
      ::close(connection_fd);
    }
  } else {
    LOG_ERROR("func:%s, accept err:%d", __FUNCTION__, errno);
    if (errno == EMFILE) {
      LOG_ERROR("func:%s, accept, sockfd reached limit err", __FUNCTION__);
    }
  }
}

}  // namespace mtd
