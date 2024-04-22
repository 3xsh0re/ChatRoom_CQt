#ifndef MY_REACTOR_H
#define MY_REACTOR_H


#include <WinSock2.h>
#include <iostream>
#include <unordered_map>
#include "my_thread_pool.hpp"
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

namespace mtd {
  class NetEnvironment {
  public:
    static void Init();
    static void Exit();
  };

  enum ListenEventType { Accept = 1, Read = 2, Write = 4, Close = 8 };

  struct ListenEvent {
    void AddAccept();
    void AddClose();
    void AddWrite();
    void AddRead();
    void RemoveAccept();
    void RemoveClose();
    void RemoveRead();
    void RemoveWrite();
    bool IsAccept() const;
    bool IsClose() const;
    bool IsRead() const;
    bool IsWrite() const;
    char events_{};
  };

  class SubReactor;

  class Event;

  class CbType {
  public:
    using ReadCbType = std::function<void(SubReactor*, SOCKET, std::string&)>;
    using WrieteCbType = std::function<void(SubReactor*, SOCKET, std::string&)>;
    using AcceptCbType = std::function<Event(SOCKET, sockaddr)>;
    using CloseCbType = std::function<void(SubReactor*, SOCKET)>;
  };

  class Event {
  public:
    Event();

    static const Event NOP;

    Event(SOCKET sock, ListenEvent events);

    Event(SOCKET sock, ListenEvent events, CbType::AcceptCbType&& accept_cb,
      CbType::CloseCbType&& close_cb, CbType::ReadCbType&& read_cb,
      CbType::WrieteCbType&& write_cb);

    ListenEvent GetListenEvent() const;

    void SetListenEvent(ListenEvent events);

    void SetAcceptCb(CbType::AcceptCbType);

    void SetCloseCb(CbType::CloseCbType);

    void SetReadCb(CbType::ReadCbType);

    void SetWriteCb(CbType::WrieteCbType);

    void RunReadCb(SubReactor* s, SOCKET client, std::string& buf);

    void RunWriteCb(SubReactor* s, SOCKET client, std::string& buf);

    Event RunAcceptCb(SOCKET client, sockaddr addr);

    void RunCloseCb(SubReactor* s, SOCKET client);

  private:
    SOCKET sock_;
    ListenEvent events_;
    CbType::AcceptCbType accept_cb_{};
    CbType::CloseCbType close_cb_{};
    CbType::ReadCbType read_cb_{};
    CbType::WrieteCbType write_cb_{};
  };

  struct DestAddr {
    DestAddr();
    DestAddr(std::string addrs, std::string port);
    sockaddr_in serv_addr_;
  };

  class MainReactor;

  class SubReactor {
  public:
    friend void mtd::ThreadPool::AddTask(std::function<void()>&&);
    friend MainReactor;
    SubReactor(DestAddr dst, SOCKET serv_, int max_work_size);

    Event FindEventBySocket(SOCKET sock);
    void RemoveEventBySocket(SOCKET sokc);
    void SetEventBySocket(SOCKET sock, Event e);
    void RemoveAllEvent();
    int GetClientSize();
    void CloseSock(SOCKET client);
    void AddTask(std::function<void()>&& fn);
  private:
    void ReadSock(SOCKET client);
    void WakeUp();
    void BeWakeUp();
    void Run();
    SOCKET serv_, wake_, monitor_;
    DestAddr dst_;
    fd_set client_set_;
    mtd::ThreadPool thread_pool_;
    std::unordered_map<SOCKET, Event> sock_event_map_;
    std::shared_mutex map_mtx_;
    std::mutex fd_set_mtx_;
  };

  class Acceptor {
  public:
    Acceptor();

    Acceptor(const Acceptor&) = default;

    Acceptor(CbType::AcceptCbType&& accept_cb);

    void SetAcceptor(CbType::AcceptCbType&& accept_cb);

    Event RunAcceptCb(SOCKET client, sockaddr addr);

  private:
    CbType::AcceptCbType accept_cb_;
  };

  class MainReactor {
  public:
    MainReactor(std::string addr, std::string port, int max_subreactor_size,
      int max_client_size,
      CbType::AcceptCbType accept_cb);
    void SetAcceptCb(CbType::AcceptCbType accept_cb);

    ~MainReactor();

    void BindAndListenServer();

    void Run();

  private:
    void Despatch(SOCKET client, Event event);
    Acceptor acceptor_;
    DestAddr dst_;
    SOCKET serv_;
    std::list<SubReactor*> subreactor_list_;
    int max_subreactor_size_;
    int max_client_size_;
    int avg_work_size_;
  };
}  // namespace mtd
#endif