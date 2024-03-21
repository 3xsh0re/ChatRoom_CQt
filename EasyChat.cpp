#include "EasyChat.h"
#include "ui_EasyChat.h"
#include "msgitem.h"
#include "info.h"
extern Widget*pW;

const int BUFFER_SIZE = 4096; // 缓冲区大小
char spl = 20; //用户私聊消息分隔符
char spl2 = 21;//群聊消息分隔符
char spl3 = 22;//用户退出
char spl4 = 23;//新用户加入

QString now_content = "";//接受的消息内容
QStringList Treated_Msg;//处理后的消息列表

// 2、创建socket
SOCKET cliSock; // 面向网路的流式套接字,第三个参数代表自动选择协议

DWORD WINAPI recvMsgThread(LPVOID IpParameter); // 消息接收处理线程

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::reciveLogin()
{
    //连接服务器
    // 1、初始化socket库
    WSADATA wsaData;                      // 获取版本信息，说明要使用的版本
    WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(主版本号, 副版本号)

    // 2、绑定
    cliSock = socket(AF_INET, SOCK_STREAM, 0);

    // 3、打包地址
    // 客户端
    SOCKADDR_IN cliAddr = {0};
    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP地址
    // cliAddr.sin_port = htons(12224);                // 端口号
    // 服务端
    SOCKADDR_IN servAddr = {0};
    servAddr.sin_family = AF_INET;                          // 和服务器的socket一样，sin_family表示协议簇，一般用AF_INET表示TCP/IP协议。
    servAddr.sin_addr.S_un.S_addr = inet_addr(info::IP.toStdString().c_str()); // 服务端地址设置为本地回环地址
    servAddr.sin_port = htons(info::Port.toInt());                       // host to net short 端口号设置为12345
    ::connect(cliSock, (SOCKADDR *)&servAddr, sizeof(SOCKADDR));

    send(cliSock,info::name.toStdString().c_str(),info::name.length()*3,0);

    //接收联系人表
    QString list_str;
    char buffer[BUFFER_SIZE] = {0};                       // 字符缓冲区，用于接收和发送消息
    recv(cliSock, buffer, sizeof(buffer), 0);
    list_str = buffer;
    this->User_list = list_str.split("+");
    this->User_list.removeOne("");
    if (User_list.count(info::name) > 1)
    {
        QMessageBox::critical(this,tr("Warning!!!"),tr("用户名重复，请重新登录！！！"),QMessageBox::Save,QMessageBox::Save);
        closesocket(cliSock);
        this->close();
    }
    else
    {
        //创建初始联系人列表
        auto pMsgItem = new MsgItem();
        pMsgItem->setName("@群聊@");
        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint( QSize(250, 55));
        ui->UserList->addItem(item);
        ui->UserList->setItemWidget(item,pMsgItem);
        //改变聊天对象
        connect(pMsgItem,SIGNAL(Change_Chat(QString)),this,SLOT(toBox(QString)));


        for (int i = 0; i < this->User_list.size(); ++i)
        {
            auto pMsgItem2 = new MsgItem();
            if (i == this->User_list.size() - 1)
            {
                pMsgItem2->setName(this->User_list.at(i)+"(自己)");
            }
            else
            {
                pMsgItem2->setName(this->User_list.at(i));
            }
            QListWidgetItem *item2 = new QListWidgetItem;
            item2->setSizeHint( QSize(250, 55));
            ui->UserList->addItem(item2);
            ui->UserList->setItemWidget(item2,pMsgItem2);
            connect(pMsgItem2,SIGNAL(Change_Chat(QString)),this,SLOT(toBox(QString)));
        }

        // 创建接受消息线程
        RecvThread *rec = new RecvThread();
        rec->start();

        //发消息
        connect(ui->SendButton,&QPushButton::clicked,this,&Widget::Send_Message);

        connect(rec,SIGNAL(flush()),this,SLOT(Flush_Ui()));

        this->show();
    }
}
//改变聊天对象
void Widget::toBox(QString chatUser)
{
    ui->Top_show->setText("现在的聊天对象是："+chatUser);
    this->now_chat_user = chatUser;
}

//发送消息
void Widget::Send_Message()
{
    if (now_chat_user=="@群聊@")
    {
        QString Send_Message = spl2+ui->SendBox->toPlainText();
        send(cliSock,Send_Message.toStdString().c_str(),Send_Message.length()*3,0);
        ui->SendBox->setPlainText("");
    }
    else
    {
        QString Send_Message = spl+this->now_chat_user+spl+ui->SendBox->toPlainText();
        send(cliSock,Send_Message.toStdString().c_str(),Send_Message.length()*3,0);
        if (now_chat_user != info::name)
        {
            ui->Messagebox->append("You Send to：" + this->now_chat_user + "\n" + ui->SendBox->toPlainText()+ "\n");
        }
        ui->SendBox->setPlainText("");
    }
}

void Widget::Flush_Ui()
{
    // 创建一个 QTextCursor 并设置为文本末尾
    QTextCursor cursor(ui->Messagebox->textCursor());
    cursor.movePosition(QTextCursor::End);

    // 创建一个 QTextCharFormat 并设置字体格式
    QTextCharFormat format;
    format.setFontFamily("宋体");
    format.setFontPointSize(16);
    format.setFontWeight(QFont::Bold);

    // 追加文本到 QTextBrowser
    if(now_content.at(0) == spl)
    {
        format.setForeground(QColor(Qt::blue));
        // 应用字体格式到光标处的文本
        cursor.setCharFormat(format);

        Treated_Msg = now_content.split(spl);
        cursor.insertText("You Recv Message From " + Treated_Msg[1] + "：\n");
        format.setForeground(QColor(Qt::black));
        cursor.setCharFormat(format);
        cursor.insertText(Treated_Msg[2]+"\n");
    }
    else if (now_content.at(0) == spl2)
    {
        format.setForeground(QColor(Qt::blue));
        // 应用字体格式到光标处的文本
        cursor.setCharFormat(format);

        Treated_Msg = now_content.split(spl2);
        cursor.insertText(Treated_Msg[1] + " say in chatRoom：\n");
        format.setForeground(QColor(Qt::black));
        cursor.setCharFormat(format);
        cursor.insertText(Treated_Msg[2]+"\n");
    }
    else if(now_content.at(0) == spl3)//用户退出
    {
        Treated_Msg = now_content.split(spl3);
        int index = this->User_list.indexOf(Treated_Msg[1]);//找到用户位置
        ui->UserList->takeItem(index + 1);
        qDebug()<<this->User_list;
        ui->Messagebox->append("----------------------------" + Treated_Msg[1] + " 退出了聊天室----------------------------\n");
        this->User_list.removeOne(Treated_Msg[1]);
    }
    else if(now_content.at(0) == spl4)//刷新用户列表,增添新进的用户
    {
        Treated_Msg = now_content.split(spl4);
        auto pMsgItem = new MsgItem();
        pMsgItem->setName(Treated_Msg[1]);
        this->User_list.append(Treated_Msg[1]);//添加新用户
        QListWidgetItem *item = new QListWidgetItem;

        //ui改变
        ui->Messagebox->append("----------------------------" + Treated_Msg[1] + " 进入了聊天室----------------------------\n");
        item->setSizeHint( QSize(250, 55));
        ui->UserList->addItem(item);
        ui->UserList->setItemWidget(item,pMsgItem);
        connect(pMsgItem,SIGNAL(Change_Chat(QString)),this,SLOT(toBox(QString)));
    }
    now_content.clear();
}

void RecvThread::run()
{
    while (1)
    {
        char buffer[BUFFER_SIZE] = {0};                       // 字符缓冲区，用于接收和发送消息
        int nrecv = recv(cliSock, buffer, sizeof(buffer), 0); // nrecv是接收到的字节数
        if (nrecv > 0)                                        // 如果接收到的字符数大于0
        {
            now_content.append(buffer);
            emit flush();
        }
        else if (nrecv < 0) // 如果接收到的字符数小于0就说明断开连接
        {
            break;
        }
    }
}
