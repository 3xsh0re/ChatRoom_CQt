#ifndef ADDRESS_H
#define ADDRESS_H

#include <iostream>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)
#elif __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#endif

namespace mtd {

class InetAddress {
 public:
  explicit InetAddress(const sockaddr_in& addr) : addr_(addr) {}

  explicit InetAddress() {}

  explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");

  void SetAddress(uint16_t port, std::string ip = "127.0.0.1");

  void SetAddress(const sockaddr_in& addr) { addr_ = addr; }

  std::string ToIp() const;

  std::string ToIpPort() const;

  uint16_t ToPort() const;

  sockaddr* GetSockAddr() const { return (sockaddr*)&addr_; }

 private:
  sockaddr_in addr_;
};

}  // namespace mtd

#endif