#ifndef WECHAT_H
#define WECHAT_H

#include <customwidget.h>
#include<QCloseEvent>
#include<QVBoxLayout>
#include<QMenu>
namespace Ui {
class WeChat;
}

class WeChat : public CustomMoveDialog
{
    Q_OBJECT

public:
    explicit WeChat(QWidget *parent = 0);
    ~WeChat();
signals:
    void SIG_close();
    void SIG_addFriend();
    void SIG_createRoom();
    void SIG_joinRoom();
    void SIG_ChangeStyle(QString style);
public slots:

    void on_pb_friend_clicked();

    void on_pb_group_clicked();

    void on_pb_space_clicked();

    void on_pb_menu_clicked();

    void slot_addUser(QWidget *item);

    void slot_deleteUser(QWidget *item);

    void slot_setInfo( int id, QString name ,
                       QString icon = QString(":/tx/12.png"),
                       QString feeling = QString("很懒,什么也没写") );

    void slot_dealMenu(QAction *action);
    void slot_dealStyleMenu(QAction *action);
private:
    Ui::WeChat *ui;
    QVBoxLayout* m_mainLayout;
public:
    QMenu * m_mainMenu;
    QMenu * m_styleMenu;

public:
    int m_id;
    QString m_name;
    QString m_icon;
    QString m_feeling;
private slots:
    void on_pb_close_clicked();
    void on_pb_min_clicked();
    void on_pb_style_clicked();
};

#endif // WECHAT_H
