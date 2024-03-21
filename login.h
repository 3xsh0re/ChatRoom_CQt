#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include<QMessageBox>
#include<QRegularExpression>//正则
#include<winsock2.h>

namespace Ui {
class login;
}

class login : public QWidget
{
    Q_OBJECT
public:
    explicit login(QWidget *parent = nullptr);
    ~login();
    void Login_Success();
signals:
    void Jump_to_Chat();

private:
    Ui::login *ui;
};

#endif // LOGIN_H
