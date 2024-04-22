#include <WinSock2.h>
#include <ws2tcpip.h>
#include "my_reactor.h"
#include <unordered_set>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

// ----------------------------globol value------------------------------------
constexpr int kMaxClientSize = 60;
constexpr int kMaxBufferSize = 1024;
std::unordered_map<SOCKET, std::string> socket_name_map;
std::unordered_map<std::string, SOCKET> name_socket_map;
std::shared_mutex map_mtx;
// ----------------------------globol value------------------------------------

void BoardCaseNoSelf(SOCKET client, std::string& s) {
  {
    std::shared_lock<std::shared_mutex> slock(map_mtx);
    for (auto& it : name_socket_map) {
      if (it.second != client)
        send(it.second, s.c_str(), s.size(), 0);
    }
  }
}

void BoardCase(std::string& s) {
  {
    std::shared_lock<std::shared_mutex> slock(map_mtx);
    for (auto& it : name_socket_map) {
      send(it.second, s.c_str(), s.size(), 0);
    }
  }
}

bool IsNewClient(SOCKET client) {
  return socket_name_map[client] == "";
}

bool IsNewName(std::string& s) {
  return name_socket_map.find(s) == name_socket_map.end();
}

bool IsPrivateChat(std::string& s) {
  /*
    私密聊天msg格式: (c->s)
    char(20) + dest_name + char(20) + msg
  */
  return s[0] == 20;
}

bool IsGroupChat(std::string& s) {
  /*
    群发格式：(c->s)
    char(21) + msg
  */
  return s[0] == 21;
}

void SendPrivateMsg(SOCKET client, std::string& buf) {
  std::string dest_name, msg;
  int begin = 1;
  while (buf[begin] != 20)
    dest_name += buf[begin++];
  std::string source_name = socket_name_map[client];
  /*
      私密聊天msg格式: (s->c)
      char(20) + source_name + char(20) + msg
  */
  msg.append(1, char(20)).append(
    source_name.c_str()).append(
      1, char(20)).append(
        buf.c_str() + begin + 1, buf.size() - begin - 1);
  // 服务端显示

  if (name_socket_map.find(dest_name.c_str()) != name_socket_map.end()) {
    send(name_socket_map[dest_name], msg.c_str(), msg.size(), 0);
    std::cout << "私聊: " << socket_name_map[client] << " -> " << dest_name << " msg: " << buf.c_str() + begin + 1 << '\n';
  }

}

void SendGroupMsg(mtd::SubReactor* s, SOCKET client, std::string& buf) {
  std::string source_name = socket_name_map[client];
  std::string msg;
  /*
    群发聊天msg格式: (s->c)
    char(21) + source_name + char(21) + msg
  */
  msg.append(1, char(21)).append(
    source_name.c_str()).append(
      1, char(21)).append(
        buf.c_str() + 1, buf.size() - 1);
  std::function<void()> fn = [msg]() ->void {
    std::string Msg = msg;
    BoardCase(Msg);
    };
  s->AddTask(std::move(fn));
}
void UserLogin(mtd::SubReactor* s, SOCKET client, std::string& buf) {
  std::string all_user_name;
  {
    std::shared_lock<std::shared_mutex> slock(map_mtx);
    for (auto& name : name_socket_map) {
      all_user_name.append(name.first.c_str()).append("+");
    }
  }

  all_user_name.append(buf).append("+");

  send(client, all_user_name.c_str(), all_user_name.size(), 0);//给用户初始化对话列表使用

  if (IsNewName(buf)) {
    /*
      用户进入msg格式： (s->c)
      char(23) + user_name
    */
    std::string msg;
    msg.append(1, char(23)).append(buf.c_str());
    std::function<void()> fn = [client, msg]() ->void {
      std::string Msg = msg;
      BoardCaseNoSelf(client, Msg);
      };
    s->AddTask(std::move(fn));
    {
      std::unique_lock<std::shared_mutex> lock(map_mtx);
      socket_name_map[client] = buf;
      name_socket_map[buf] = client;
    }
  }
  else {
    s->CloseSock(client);
  }
}

void ReadCb(mtd::SubReactor* s, SOCKET client, std::string& buf) {
  if (IsNewClient(client)) { //初次登录
    UserLogin(s, client, buf);
  }
  else if (IsPrivateChat(buf)) {
    SendPrivateMsg(client, buf);
  }
  else if (IsGroupChat(buf)) {
    SendGroupMsg(s, client, buf);
  }
}

void CloseCb(mtd::SubReactor* s, SOCKET client) {
  std::cout << "close : " << client << '\n';
  /*
        用户退出msg格式： (s->c)
        char(22) + user_name
  */
  if (socket_name_map[client] != "") {//正常退出
    std::string msg;
    msg.append(1, char(22)).append(socket_name_map[client].c_str());
    std::function<void()> fn = [client, msg]() ->void {
      std::string Msg = msg;
      BoardCaseNoSelf(client, Msg);
      };
    s->AddTask(std::move(fn));
    {
      std::unique_lock<std::shared_mutex> slock(map_mtx);
      std::string name = socket_name_map[client];
      name_socket_map.erase(name);
      socket_name_map.erase(client);
    }
  }
  else { //重名退出
    std::cout << "rapead name close\n";
    {
      std::unique_lock<std::shared_mutex> slock(map_mtx);
      socket_name_map.erase(client);
    }
  }
}

mtd::Event AcceptCb(SOCKET client, sockaddr addr) {
  {
    std::unique_lock<std::shared_mutex> lock(map_mtx);
    socket_name_map[client] = "";
  }
  std::cout << "accept : " << client << '\n';
  mtd::ListenEvent le;
  le.AddRead();
  le.AddClose();
  mtd::Event e(client, le);
  e.SetReadCb(ReadCb);
  e.SetCloseCb(CloseCb);
  return e;
}

int main() {
  mtd::CbType::AcceptCbType accept_cb = AcceptCb;
  mtd::MainReactor main_reactor("127.0.0.1", "12345", 2, kMaxClientSize, accept_cb);
  main_reactor.Run();
  return 0;
}