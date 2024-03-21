#include "login.h"
#include "ui_login.h"
#include "info.h"

login::login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);
    connect(ui->StartButton,&QPushButton::clicked,this,&login::Login_Success);
}

login::~login()
{
    delete ui;
}

void login::Login_Success()
{
    //获取IP和用户名
    QString server_IP = ui->IP_input->text();
    QString username  = ui->name_input->text();
    QString server_port = ui->Port_input->text();
    //正则匹配IP和Port
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QRegularExpressionMatch match1 = ipRegex.match(server_IP);
    QRegularExpression portRegex("^([8-9][0-9]{1,3}|[1-5][0-9]{4}|6[0-5][0-9]{3}|6553[0-5]|655[0-2][0-9])$");
    QRegularExpressionMatch match2 = portRegex.match(server_port);

    if (match1.hasMatch()&&match2.hasMatch()&&username!=""&&username.length()<=4)
    {
        info::name = username;
        info::IP = server_IP;
        info::Port = server_port;
        this->hide();
        emit Jump_to_Chat();
    }
    else
    {
        QMessageBox::critical(this,tr("Warning!!!"),tr("请核对用户名(小于5个字)和IP!!!"),QMessageBox::Save,QMessageBox::Save);
    }
}
