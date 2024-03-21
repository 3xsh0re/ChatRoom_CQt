#include "msgitem.h"
#include "ui_msgitem.h"

MsgItem::MsgItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MsgItem)
{
    ui->setupUi(this);
    connect(ui->MsgBox,&QPushButton::clicked,this,&MsgItem::Change);
}

MsgItem::~MsgItem()
{
    delete ui;
}

void MsgItem::Change()
{
    emit Change_Chat(name());
}
QString MsgItem::name() const
{
    return ui->MsgBox->text();
}

void MsgItem::setName(const QString& name)
{
    ui->MsgBox->setText(name);
}


QString MsgItem::content() const
{
    return ui->MsgBox->text();
}

void MsgItem::setContent(const QString& name)
{
    ui->MsgBox->setText(name);
}
