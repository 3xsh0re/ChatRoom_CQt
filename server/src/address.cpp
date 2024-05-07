#include "address.h"

namespace mtd {
InetAddress::InetAddress(uint16_t port, std::string ip) {
  SetAddress(port, ip);
}

void InetAddress::SetAddress(uint16_t port, std::string ip) {
  ::memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
  addr_.sin_port = ::htons(port);
}

std::string InetAddress::ToIp() const {
  char buf[64]{};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  return std::string(buf);
}

std::string InetAddress::ToIpPort() const {
  char buf[64]{};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  ::sprintf(buf + ::strlen(buf), ":%u", ::ntohs(addr_.sin_port));
  return std::string(buf);
}

uint16_t InetAddress::ToPort() const { return ::ntohs(addr_.sin_port); }

}  // namespace mtd
