#ifndef EASYCHAT_H
#define EASYCHAT_H

#include <QWidget>
#include<QPushButton>
#include<winsock2.h>
#include<QThread>
#include<QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

    QString now_chat_user;//当前聊天对象

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void Send_Message();
    QString Recv_Message();
    QStringList User_list;

private slots:
    void reciveLogin();
    void Flush_Ui();
    void toBox(QString i);//改变聊天对象

private:
    Ui::Widget *ui;
};

class RecvThread:public QThread
{
    Q_OBJECT

signals:
    void flush();

public:
    void run();
};

#endif // EASYCHAT_H
