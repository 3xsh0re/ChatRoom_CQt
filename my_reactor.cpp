#include "my_reactor.h"

namespace mtd {
  //NetEnvironment
  void NetEnvironment::Init() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
      std::cerr << "WSAStartup failed\n";
    }
    else {
      std::cout << "initialization succeeded\n";
    }
  }

  void NetEnvironment::Exit() {
    WSACleanup();
  }
  //NetEnvironment


  //ListenEvent
  void ListenEvent::AddAccept() { events_ |= ListenEventType::Accept; }
  void ListenEvent::AddClose() { events_ |= ListenEventType::Close; }
  void ListenEvent::AddWrite() { events_ |= ListenEventType::Write; }
  void ListenEvent::AddRead() { events_ |= ListenEventType::Read; }
  void ListenEvent::RemoveAccept() { events_ &= ~ListenEventType::Accept; }
  void ListenEvent::RemoveClose() { events_ &= ~ListenEventType::Close; }
  void ListenEvent::RemoveWrite() { events_ &= ~ListenEventType::Write; }
  void ListenEvent::RemoveRead() { events_ &= ~ListenEventType::Read; }
  bool ListenEvent::IsAccept() const { return events_ & ListenEventType::Accept; }
  bool ListenEvent::IsClose() const { return events_ & ListenEventType::Close; }
  bool ListenEvent::IsRead() const { return events_ & ListenEventType::Read; }
  bool ListenEvent::IsWrite() const { return events_ & ListenEventType::Write; }
  //ListenEvent

  //Event
  const Event Event::NOP;

  Event::Event()
    : sock_(), events_(), accept_cb_(nullptr), close_cb_(nullptr),
    read_cb_(nullptr), write_cb_(nullptr) {}

  Event::Event(::SOCKET sock, ListenEvent events)
    : sock_(sock), events_(events), accept_cb_(nullptr), close_cb_(nullptr),
    read_cb_(nullptr), write_cb_(nullptr) {}

  Event::Event(::SOCKET sock, ListenEvent events,
    CbType::AcceptCbType&& accept_cb, CbType::CloseCbType&& close_cb,
    CbType::ReadCbType&& read_cb, CbType::WrieteCbType&& write_cb)
    : sock_(sock), events_(events), accept_cb_(std::move(accept_cb)),
    close_cb_(std::move(close_cb)), read_cb_(std::move(read_cb)),
    write_cb_(std::move(write_cb)) {}

  ListenEvent Event::GetListenEvent() const {
    return events_;
  }

  void Event::SetListenEvent(ListenEvent events) { events_ = events; }

  void Event::SetAcceptCb(CbType::AcceptCbType accept_cb) {
    accept_cb_ = std::move(accept_cb);
  }

  void Event::SetCloseCb(CbType::CloseCbType close_cb) {
    close_cb_ = std::move(close_cb);
  }

  void Event::SetReadCb(CbType::ReadCbType read_cb) {
    read_cb_ = std::move(read_cb);
  }

  void Event::SetWriteCb(CbType::WrieteCbType write_cb) {
    write_cb_ = std::move(write_cb);
  }

  void Event::RunReadCb(SubReactor* s, SOCKET client, std::string& buf) {
    if (read_cb_ != nullptr)
      read_cb_(s, client, buf);
  }

  void Event::RunWriteCb(SubReactor* s, SOCKET client, std::string& buf) {
    if (write_cb_ != nullptr)
      write_cb_(s, client, buf);
  }

  Event Event::RunAcceptCb(SOCKET client, sockaddr addr) {
    if (accept_cb_ != nullptr)
      return accept_cb_(client, addr);
    else
      return Event::NOP;
  }

  void Event::RunCloseCb(SubReactor* s, SOCKET client) {
    if (close_cb_ != nullptr)
      close_cb_(s, client);
  }

  //Event

  //DestAddr
  DestAddr::DestAddr() : serv_addr_{} {}

  DestAddr::DestAddr(std::string addrs, std::string port) {
    serv_addr_.sin_family = AF_INET;
    serv_addr_.sin_addr.s_addr = inet_addr(addrs.c_str());
    serv_addr_.sin_port = ::htons(::atoi(port.c_str()));
  }
  //DestAddr

  //SubReactor
  SubReactor::SubReactor(DestAddr dst, SOCKET serv, int max_work_size)
    : dst_(dst), serv_(serv), thread_pool_(max_work_size), client_set_{}, sock_event_map_{}
  {
    wake_ = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(wake_, (sockaddr*)&dst_.serv_addr_, sizeof(dst_.serv_addr_)) == SOCKET_ERROR) {
      std::cerr << "Failed to connect to server: " << WSAGetLastError() << std::endl;
      ::closesocket(wake_); // 关闭套接字
      NetEnvironment::Exit();
    }
    DestAddr tmp{};
    int tmp_size = sizeof(tmp.serv_addr_);
    monitor_ = ::accept(serv_, (sockaddr*)&tmp.serv_addr_, &tmp_size);
    ListenEvent le{};
    Event event(monitor_, le);
    std::cout << "monitor_ : " << monitor_ << '\n';
    sock_event_map_[monitor_] = event;
    {
      std::lock_guard<std::mutex> lock(fd_set_mtx_);
      FD_SET(monitor_, &client_set_);
    }
  }

  void SubReactor::AddTask(std::function<void()>&& fn) {
    thread_pool_.AddTask(std::move(fn));
  }

  void SubReactor::WakeUp() {
    send(wake_, "wake up", 7, 0);
  }

  void SubReactor::BeWakeUp() {
    char buffer[7]{};
    int buf_len;
    buf_len = recv(monitor_, buffer, 7, 0);
  }

  void SubReactor::ReadSock(SOCKET client) {
    Event e{};
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx_);
      e = sock_event_map_[client];
    }
    char buf[1024]{};
    int msg_len = recv(client, buf, 1024, 0);
    if (msg_len <= 0) {
      if (e.GetListenEvent().IsClose()) {
        e.RunCloseCb(this, client);
      }
      ::closesocket(client);
      {
        std::unique_lock<std::shared_mutex> slock(map_mtx_);
        sock_event_map_.erase(client);
      }
    }
    else {
      std::string msg(buf);
      if (e.GetListenEvent().IsRead()) {
        e.RunReadCb(this, client, msg);
      }
      {
        std::shared_lock<std::shared_mutex> slock(map_mtx_);
        if (sock_event_map_.find(client) != sock_event_map_.end()) {
          {
            std::lock_guard<std::mutex> lock(fd_set_mtx_);
            FD_SET(client, &client_set_);
          }
        }
      }
      WakeUp();
    }
  }

  void SubReactor::CloseSock(SOCKET client) {
    Event e{};
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx_);
      if (sock_event_map_.find(client) == sock_event_map_.end())
        return;
      e = sock_event_map_[client];
    }
    if (e.GetListenEvent().IsClose()) {
      e.RunCloseCb(this, client);
    }
    ::closesocket(client);
    {
      std::unique_lock<std::shared_mutex> slock(map_mtx_);
      sock_event_map_.erase(client);
    }
    {
      std::lock_guard<std::mutex> lock(fd_set_mtx_);
      FD_CLR(client, &client_set_);
    }
    WakeUp();
  }

  void SubReactor::Run() {
    std::function<void()> loop = [this]() {
      fd_set copy{};
      SOCKET current_sock{};
      int count{};
      while (1) {
        FD_ZERO(&copy);
        {
          std::lock_guard<std::mutex> lock(fd_set_mtx_);
          copy = client_set_;
        }
        count = select(-1, &copy, nullptr, nullptr, nullptr);
        for (int i = 0; i < count; ++i) {
          current_sock = copy.fd_array[i];
          if (current_sock == monitor_) {
            BeWakeUp();
            continue;
          }
          {
            std::lock_guard<std::mutex> lock(fd_set_mtx_);
            FD_CLR(current_sock, &client_set_);
          }
          std::function<void()> task = [this, current_sock]()->void {
            ReadSock(current_sock);
            };
          thread_pool_.AddTask(std::move(task));
        }
      }
      };
    thread_pool_.AddTask(std::move(loop));
  }

  Event SubReactor::FindEventBySocket(SOCKET sock) {
    Event result{};
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx_);
      if (sock_event_map_.find(sock) != sock_event_map_.end()) {
        result = sock_event_map_[sock];
      }
      else {
        result = Event::NOP;
      }
    }
    return result;
  }

  void SubReactor::RemoveAllEvent() {
    {
      std::unique_lock<std::shared_mutex> lock(map_mtx_);
      sock_event_map_.clear();
    }
  }

  void SubReactor::SetEventBySocket(SOCKET sock, Event e) {
    {
      std::unique_lock<std::shared_mutex> lock(map_mtx_);
      sock_event_map_[sock] = e;
    }
    {
      std::lock_guard<std::mutex> lock(fd_set_mtx_);
      if (!FD_ISSET(sock, &client_set_)) {
        FD_SET(sock, &client_set_);
        WakeUp();
      }
    }
  }

  void SubReactor::RemoveEventBySocket(SOCKET sock) {
    {
      {
        std::unique_lock<std::shared_mutex> lock(map_mtx_);
        sock_event_map_.erase(sock);
      }
    }
  }

  int SubReactor::GetClientSize() {
    {
      {
        std::unique_lock<std::shared_mutex> lock(map_mtx_);
        return sock_event_map_.size();
      }
    }
  }
  //SubReactor

  //Acceptor
  Acceptor::Acceptor()
    : accept_cb_(nullptr) {}

  Acceptor::Acceptor(
    CbType::AcceptCbType&& accept_cb)
    : accept_cb_(std::move(accept_cb)) {}

  void  Acceptor::SetAcceptor(CbType::AcceptCbType&& accept_cb) {
    accept_cb_ = accept_cb;
  }

  Event Acceptor::RunAcceptCb(SOCKET client, sockaddr addr) {
    if (accept_cb_ != nullptr)
      return accept_cb_(client, addr);
    else
      return Event::NOP;
  }

  //Acceptor

  //MainReactor
  void MainReactor::BindAndListenServer() {
    serv_ = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(serv_, (struct sockaddr*)&dst_.serv_addr_, sizeof(dst_.serv_addr_)) < 0) {
      std::cerr << "bind error\n";
    }
    else {
      std::cout << "server create succeeded\n";
    }
    if (::listen(serv_, max_client_size_) == SOCKET_ERROR) {
      std::cerr << "Listen failed\n";
      closesocket(serv_);
    }
    else {
      std::cout << "listen succeeded\n";
    }
  }

  MainReactor::MainReactor(
    std::string addr, std::string port, int max_subreactor_size, int max_client_size,
    std::function<Event(SOCKET client, sockaddr addr)> accept_cb)
    : dst_(addr, port),
    acceptor_(std::move(accept_cb)), max_subreactor_size_(max_subreactor_size),
    max_client_size_(max_client_size), avg_work_size_(max_client_size / (max_subreactor_size * 2)) {

    NetEnvironment::Init();

    BindAndListenServer();
  }

  MainReactor::~MainReactor() {
    NetEnvironment::Exit();
  }

  void MainReactor::SetAcceptCb(CbType::AcceptCbType cb) {
    acceptor_.SetAcceptor(std::move(cb));
  }

  void MainReactor::Despatch(SOCKET client, Event event) {
    auto min_client_sub = subreactor_list_.front();
    for (auto& it : subreactor_list_) {
      if (it->GetClientSize() < min_client_sub->GetClientSize())
        min_client_sub = it;
    }
    min_client_sub->SetEventBySocket(client, event);
  }

  void MainReactor::Run() {
    for (int i = 0; i < max_subreactor_size_; ++i) {
      subreactor_list_.push_back(new SubReactor(dst_, serv_, avg_work_size_));
      subreactor_list_.back()->Run();
    }

    SOCKET client;
    sockaddr addr;
    int add_size = sizeof(addr);
    Event event{};
    while (1) {
      client = ::accept(serv_, &addr, &add_size);
      event = acceptor_.RunAcceptCb(client, addr);
      Despatch(client, event);
    }
  }
  //MainReactor
} // namespace mtd
