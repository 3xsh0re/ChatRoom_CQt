#include <iostream>
#include <Winsock2.h> //socket头文件
#include <cstring>
#include <thread>
#include <algorithm>

using namespace std;

#pragma comment(lib, "ws2_32.lib") // socket库

//==============================全局变量区===================================
const int BUFFER_SIZE = 1024 * 10;      // 缓冲区大小
int RECV_TIMEOUT = 10;             // 接收消息超时
int SEND_TIMEOUT = 10;             // 发送消息超时
const int WAIT_TIME = 10;          // 每个客户端等待事件的时间，单位毫秒
const int MAX_LINK_NUM = 10;       // 服务端最大链接数
SOCKET cliSock[MAX_LINK_NUM]{};      // 客户端套接字 0号为服务端
SOCKADDR_IN cliAddr[MAX_LINK_NUM]{}; // 客户端地址
WSAEVENT cliEvent[MAX_LINK_NUM]{};   // 客户端事件 0号为服务端,它用于让程序的一部分等待来自另一部分的信号。例如，当数据从套接字变为可用时，winsock 库会将事件设置为信号状态
int total = 0;                     // 当前已经链接的客服端服务数

//==============================函数声明===================================

void ServFun(SOCKET servSock); // 服务器端函数声明 while(1) 不退出

int main()
{
    // 1、初始化socket库
    WSADATA wsaData;                      // 获取版本信息，说明要使用的版本
    WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(主版本号, 副版本号)

    // 2、创建socket
    SOCKET servSock = socket(AF_INET, SOCK_STREAM, 0); // 面向网路的流式套接字

    // 3、将服务器地址打包在一个结构体里面
    SOCKADDR_IN servAddr;                                   // sockaddr_in 是internet环境下套接字的地址形式
    servAddr.sin_family = AF_INET;                          // 和服务器的socket一样，sin_family表示协议簇，一般用AF_INET表示TCP/IP协议。
    //servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 服务端地址设置为本地回环地址
    servAddr.sin_addr.S_un.S_addr = ADDR_ANY;// 服务端地址设置为本地回环地址
    servAddr.sin_port = htons(12345);                       // host to net short 端口号设置为12345

    // 4、绑定服务端的socket和打包好的地址
    bind(servSock, (SOCKADDR *)&servAddr, sizeof(servAddr));

    // 4.5给服务端sokect绑定一个事件对象，用来接收客户端链接的事件
    WSAEVENT servEvent = WSACreateEvent();              // 创建一个人工重设为传信的事件对象
    WSAEventSelect(servSock, servEvent, FD_ALL_EVENTS); // 绑定事件对象，并且监听所有事件

    cliSock[0] = servSock;
    cliEvent[0] = servEvent;

    // 5、开启监听
    listen(servSock, 10); // 监听队列长度为10

    ServFun(servSock);

    WSACleanup();
    return 0;
}

void ServFun(SOCKET servSock)  
{
    cout << "聊天室服务器已开启" << endl;
    // 该线程负责处理服务端和各个客户端发生的事件
    // 将传入的参数初始化
    string usrs[MAX_LINK_NUM];
    while (1)                                 // 不停执行
    {
        for (int i = 0; i < total + 1; i++) // i代表现在正在监听事件的终端
        {
            // 若有一个客户端链接，total==1，循环两次，包含客户端和服务端
            // 对每一个终端（客户端和服务端），查看是否发生事件，等待WAIT_TIME毫秒
            int index = WSAWaitForMultipleEvents(1, &cliEvent[i], false, WAIT_TIME, 0);

            index -= WSA_WAIT_EVENT_0; // 此时index为发生事件的终端下标

            if (index == WSA_WAIT_TIMEOUT || index == WSA_WAIT_FAILED)
            {
                continue; // 如果出错或者超时，即跳过此终端
            }

            else if (index == 0)
            {
                WSANETWORKEVENTS networkEvents;
                WSAEnumNetworkEvents(cliSock[i], cliEvent[i], &networkEvents); // 查看是什么事件
                // 事件选择
                if (networkEvents.lNetworkEvents & FD_ACCEPT) // 若产生accept事件（此处与位掩码相与）
                {
                    if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
                    {
                        cout << "连接时产生错误，错误代码" << networkEvents.iErrorCode[FD_ACCEPT_BIT] << endl;
                        continue;
                    }
                    // 接受链接
                    if (total + 1 < MAX_LINK_NUM) // 若增加一个客户端仍然小于最大连接数，则接受该链接
                    {
                        // total为已连接客户端数量
                        int nextIndex = total + 1; // 分配给新客户端的下标
                        int addrLen = sizeof(SOCKADDR);
                        SOCKET newSock = accept(servSock, (SOCKADDR *)&cliAddr[nextIndex], &addrLen);

                        if (newSock != INVALID_SOCKET)
                        {
                            cliSock[nextIndex] = newSock;
                            // 新客户端的地址已经存在cliAddr[nextIndex]中了
                            total++; // 客户端连接数增加
                            // 接收用户名称信息
                            char usr_name_char[BUFFER_SIZE]{};
                            while (1)
                            {
                                int nrecv = recv(cliSock[nextIndex], usr_name_char, sizeof(usr_name_char), 0);
                                if (nrecv > 0)
                                {
                                    usrs[nextIndex] = string(usr_name_char);
                                    //给新用户传送所有在聊天室的用户的用户名，方便初始化
                                    string all_user_name;
                                    for (int i = 1; i <= total; ++i) //包括自己
                                    {
                                        all_user_name.append(usrs[i].c_str()).append("+");
                                    }
                                    send(cliSock[nextIndex], all_user_name.c_str(), all_user_name.size(), 0);

                                    if (std::count(usrs + 1, usrs + total + 1, usrs[nextIndex]) != 2) //不重名才广播并绑定事件
                                    {
                                        cout << "用户: " << usrs[nextIndex].c_str()
                                            << " 进入了聊天室，当前连接数：" << total << endl;
                                        // 为新客户端绑定事件对象,同时设置监听，close，read，write
                                        WSAEVENT newEvent = WSACreateEvent();
                                        WSAEventSelect(cliSock[nextIndex], newEvent, FD_CLOSE | FD_READ | FD_WRITE);
                                        cliEvent[nextIndex] = newEvent;
                                        /*
                                            用户进入msg格式： (s->c) 
                                            char(23) + user_name
                                        */
                                        string msg; 
                                        msg.append(1, char(23)).append(usrs[nextIndex].c_str());
                                        for (int i = 1; i < total; ++i) //不包括自己
                                        {
                                            send(cliSock[i], msg.c_str(), msg.size(), 0);
                                        }
                                        break;
                                    } 
                                    else 
                                    {
                                        total--;
                                        usrs[nextIndex].clear();
                                        ::closesocket(newSock);
                                        cliSock[nextIndex] = INVALID_SOCKET;
                                        cliAddr[nextIndex] = SOCKADDR_IN{};
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (networkEvents.lNetworkEvents & FD_CLOSE) // 客户端被关闭，即断开连接
                {
                    // i表示已关闭的客户端下标
                    string exit_user_name(usrs[i]);
                    // 释放这个客户端的资源
                    closesocket(cliSock[i]);
                    WSACloseEvent(cliEvent[i]);
                    // 数组调整,用顺序表删除元素
                    for (int j = i; j < total; j++)
                    {
                        cliSock[j] = cliSock[j + 1];
                        cliEvent[j] = cliEvent[j + 1];
                        cliAddr[j] = cliAddr[j + 1];
                        usrs[j].assign(usrs[j + 1]);
                    }
                    //防止越界
                    cliSock[total] = SOCKET{};
                    cliEvent[total] = HANDLE{};
                    cliAddr[total] = SOCKADDR_IN{};
                    usrs[total].clear();
                    total--;

                    cout <<  "用户: " << exit_user_name.c_str() << " 退出了聊天室,当前连接数：" << total << endl;
                    // 给所有客户端发送退出聊天室的消息
                    /*
                        用户退出msg格式： (s->c) 
                        char(22) + user_name
                    */
                    string msg;
                    msg.append(1,  char(22)).append(exit_user_name.c_str());
                    for (int j = 1; j <= total; j++)
                    {
                        send(cliSock[j], msg.c_str(), msg.size(), 0);
                    }
                }
                else if (networkEvents.lNetworkEvents & FD_READ) // 接收到消息
                {
                    char buffer[BUFFER_SIZE]{}; // 字符缓冲区，用于接收和发送消息
                    for (int j = 1; j <= total; j++)
                    {
                        int nrecv = recv(cliSock[j], buffer, sizeof(buffer), 0); // nrecv是接收到的字节数
                        if (nrecv > 0)                                           // 如果接收到的字符数大于0
                        {
                            if (buffer[0] == 20) {
                                /*
                                私密聊天msg格式: (c->s)
                                    char(20) + dest_name + char(20) + msg
                                */
                                string dest_name, msg;
                                int begin = 1;
                                while(buffer[begin] != 20)
                                    dest_name += buffer[begin++];
                                int dest_id = 0;
                                for (int i = 1; i <= total; ++i) 
                                {
                                    if(!usrs[i].compare(dest_name)) 
                                    {
                                        dest_id = i;
                                        break;
                                    }
                                }
                                string source_name = usrs[i];
                                // 服务端显示
                                std::cout << "私聊: " << usrs[i] << " -> " << dest_name << " msg: " << buffer + begin + 1<< '\n';
                                /*
                                私密聊天msg格式: (s->c)
                                    char(20) + source_name + char(20) + msg
                                */
                                msg.append(1, char(20)).append(
                                    source_name.c_str()).append(
                                    1, char(20)).append(
                                    buffer + begin + 1, strlen(buffer) - begin - 1);
                                send(cliSock[dest_id], msg.c_str(), msg.size(), 0);
                            } else if (buffer[0] = 21){ 
                                /*
                                群发格式：(c->s)
                                    char(21) + msg
                                */

                                string source_name = usrs[i];
                                string msg;
                                /*
                                群发聊天msg格式: (s->c)
                                    char(21) + source_name + char(21) + msg
                                */
                                // 服务端显示
                                std::cout << "群发: "<< source_name << " -> "<< buffer + 1<< '\n';

                                msg.append(1, char(21)).append(
                                    source_name.c_str()).append(
                                    1, char(21)).append(
                                    buffer + 1, strlen(buffer) - 1);
                                // 在其他客户端显示（广播给其他客户端）
                                for (int k = 1; k <= total; k++)
                                {
                                    send(cliSock[k], msg.c_str(), msg.size(),0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}