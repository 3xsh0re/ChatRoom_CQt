#ifndef MSGITEM_H
#define MSGITEM_H

#include <QWidget>

namespace Ui {
class MsgItem;
}

class MsgItem : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName DESIGNABLE true)
    Q_PROPERTY(QString content READ content WRITE setContent DESIGNABLE true)

public:
    explicit MsgItem(QWidget *parent = nullptr);
    ~MsgItem();
    void Change();//改变聊天窗

    QString name() const;
    void setName(const QString& name);

    QString content() const;
    void setContent(const QString& content);


signals:
    void Change_Chat(QString);//改变聊天窗

private:
    Ui::MsgItem *ui;
};

#endif // MSGITEM_H
