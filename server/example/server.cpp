#include <iostream>
#include <shared_mutex>

#include "tcp_server.h"
class ChatRoomServer {
 public:
  ChatRoomServer(mtd::EventLoop *loop, const mtd::InetAddress &addr,
                 const std::string &name)
      : loop_(loop), server_(loop, addr, name) {
    server_.set_connection_cb(
        std::bind(&ChatRoomServer::OnConnection, this, std::placeholders::_1));

    server_.set_msg_cb(std::bind(&ChatRoomServer::ReadCb, this,
                                 std::placeholders::_1, std::placeholders::_2,
                                 std::placeholders::_3));
    server_.set_thread_num(3);
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const mtd::TcpConnectionPtr &conn) {
    if (conn->IsConnected()) {
      LOG_INFO("Connected up : %s", conn->get_peer_addr().ToIpPort().c_str());
    } else {
      LOG_INFO("Connected down : %s", conn->get_peer_addr().ToIpPort().c_str());
      UserExit(conn);
    }
  }

  bool IsNewClient(mtd::TcpConnectionPtr conn) {
    return conn_name_map_[conn] == "";
  }

  bool IsNewName(const std::string &s) {
    return name_conn_map_.find(s) == name_conn_map_.end();
  }

  bool IsPrivateChat(const mtd::Buffer &buf) {
    /*
      私密聊天msg格式: (c->s)
      char(20) + dest_name + char(20) + msg
    */
    return *buf.Peek() == 20;
  }

  bool IsGroupChat(const mtd::Buffer &buf) {
    /*
      群发格式：(c->s)
      char(21) + msg
    */
    return *buf.Peek() == 21;
  }

  void BroadcastNoSelf(const mtd::TcpConnectionPtr &conn, std::string &msg) {
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx);
      for (auto &it : name_conn_map_) {
        if (it.second != conn) it.second->Send(msg);
      }
    }
  }

  void Broadcast(std::string &msg) {
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx);
      for (auto &it : name_conn_map_) {
        it.second->Send(msg);
      }
    }
  }

  void UserLogin(const mtd::TcpConnectionPtr &conn, const std::string &name) {
    std::string all_user_name;
    {
      std::shared_lock<std::shared_mutex> slock(map_mtx);
      for (auto &name : name_conn_map_) {
        all_user_name.append(name.first.c_str()).append("+");
      }
    }
    all_user_name.append(name).append("+");
    conn->Send(all_user_name);
    if (IsNewName(name)) {
      /*
        用户进入msg格式： (s->c)
        char(23) + user_name
      */
      std::string msg;
      msg.append(1, char(23)).append(name.c_str());
      BroadcastNoSelf(conn, msg);
      {
        std::unique_lock<std::shared_mutex> lock(map_mtx);
        conn_name_map_[conn] = name;
        name_conn_map_[name] = conn;
      }
    } else {
      conn->Shutdown();
    }
  }

  void UserExit(const mtd::TcpConnectionPtr &conn) {
    if (conn_name_map_[conn] != "") {  // 正常退出
      std::string msg;
      msg.append(1, char(22)).append(conn_name_map_[conn].c_str());
      BroadcastNoSelf(conn, msg);
      {
        std::unique_lock<std::shared_mutex> slock(map_mtx);
        std::string name = conn_name_map_[conn];
        name_conn_map_.erase(name);
        conn_name_map_.erase(conn);
      }
    } else {  // 重名退出
      {
        std::unique_lock<std::shared_mutex> slock(map_mtx);
        conn_name_map_.erase(conn);
      }
    }
  }

  void SendPrivateMsg(const mtd::TcpConnectionPtr &conn, mtd::Buffer &buffer) {
    std::string dest_name, msg;
    std::string buf(buffer.RetriveAllAsString());
    buf = buf.substr(0, buf.find('\0'));
    int begin = 1;
    while (buf[begin] != 20) dest_name += buf[begin++];
    std::string source_name = conn_name_map_[conn];
    /*
        私密聊天msg格式: (s->c)
        char(20) + source_name + char(20) + msg
    */
    msg.append(1, char(20))
        .append(source_name.c_str())
        .append(1, char(20))
        .append(buf.c_str() + begin + 1, buf.size() - begin - 1);
    // 服务端显示

    if (name_conn_map_.find(dest_name.c_str()) != name_conn_map_.end()) {
      // int fd = name_conn_map_[dest_name]->get_fd();
      // send(fd, msg.c_str(), msg.size(), 0);
      name_conn_map_[dest_name.c_str()]->Send(msg);
      LOG_INFO("soource fd : %d, dest fd : %d",
               name_conn_map_[source_name.c_str()]->get_fd(),
               name_conn_map_[dest_name.c_str()]->get_fd());
    }
  }

  void SendGroupMsg(const mtd::TcpConnectionPtr &conn, mtd::Buffer &buffer) {
    std::string source_name = conn_name_map_[conn];
    std::string msg;
    std::string buf = buffer.RetriveAllAsString();
    /*
      群发聊天msg格式: (s->c)
      char(21) + source_name + char(21) + msg
    */
    msg.append(1, char(21))
        .append(source_name.c_str())
        .append(1, char(21))
        .append(buf.c_str() + 1, buf.size() - 1);
    Broadcast(msg);
  }

  void ReadCb(const mtd::TcpConnectionPtr &conn, mtd::Buffer &buf,
              mtd::Timestamp time) {
    if (IsNewClient(conn)) {  // 初次登录
      std::string name = buf.RetriveAllAsString();
      name = name.substr(0, name.find('\0'));
      UserLogin(conn, name);
    } else if (IsPrivateChat(buf)) {
      SendPrivateMsg(conn, buf);
    } else if (IsGroupChat(buf)) {
      SendGroupMsg(conn, buf);
    }
  }

  mtd::EventLoop *loop_;
  mtd::TcpServer server_;
  std::unordered_map<std::string, mtd::TcpConnectionPtr> name_conn_map_;
  std::unordered_map<mtd::TcpConnectionPtr, std::string> conn_name_map_;
  std::shared_mutex map_mtx;
};

int main() {
  mtd::EventLoop loop;
  mtd::InetAddress addr(12345, "127.0.0.1");
  ChatRoomServer server(&loop, addr, "server#0");
  server.Start();
  loop.Loop();
  return 0;
}