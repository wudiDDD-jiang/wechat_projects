#include "wechat.h"
#include "ui_wechat.h"
#include"qDebug"
#include<QMessageBox>

WeChat::WeChat(QWidget *parent) :
    CustomMoveDialog(parent),
    ui(new Ui::WeChat),m_id(0)
{
    ui->setupUi(this);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setContentsMargins( 3,3,3,3);
    m_mainLayout->setSpacing(1);
    ui->wdg_list->setLayout( m_mainLayout );

    m_mainMenu = new QMenu(this); // this 传参 指定父控件 , 父控件回收的时候可以回收子控件

    m_mainMenu->addAction("添加好友");
    m_mainMenu->addSeparator();
    m_mainMenu->addAction("创建房间");
    m_mainMenu->addAction("加入房间");

    connect( m_mainMenu , SIGNAL(triggered(QAction*)) , this , SLOT(slot_dealMenu(QAction* )) );

    m_styleMenu = new QMenu(this);
    m_styleMenu->addAction("换肤");
    m_styleMenu->addSeparator();
    m_styleMenu->addAction("默认灰色");
    m_styleMenu->addAction("蓝色妖姬");
    m_styleMenu->addAction("绿色原谅");
//    m_styleMenu->addAction("金色传说");
//    m_styleMenu->addAction("深色诱惑");

    connect( m_styleMenu , SIGNAL(triggered(QAction*)) , this , SLOT(slot_dealStyleMenu(QAction* )) );

}

WeChat::~WeChat()
{
    delete ui;
}


//点击好友标签
void WeChat::on_pb_friend_clicked()
{
    ui->pb_friend->setChecked(true);
    ui->pb_group->setChecked(false);
    ui->pb_space->setChecked(false);
}
//点击群标签
void WeChat::on_pb_group_clicked()
{
    ui->pb_friend->setChecked(false);
    ui->pb_group->setChecked(true);
    ui->pb_space->setChecked(false);
}

//点击空间标签
void WeChat::on_pb_space_clicked()
{
    ui->pb_friend->setChecked(false);
    ui->pb_group->setChecked(false);
    ui->pb_space->setChecked(true);
}

//添加用户
void WeChat::slot_addUser(QWidget *item)
{
    m_mainLayout->addWidget( item , 0 , Qt::AlignTop);
}

//删除用户
void WeChat::slot_deleteUser(QWidget *item)
{
    m_mainLayout->removeWidget(item);
}

//设置个人信息
void WeChat::slot_setInfo(int id, QString name, QString icon, QString feeling)
{
    m_id = id;
    m_name = name;
    m_icon = icon ;
    m_feeling = feeling;

    ui->lb_name ->setText( m_name );
    ui->pb_icon->setIcon( QIcon(m_icon) );
    ui->le_feeling->setText( m_feeling );
}


//菜单
void WeChat::on_pb_menu_clicked()
{
    if( m_mainMenu )
    {
        QPoint p = QCursor::pos() ;
        QSize size = m_mainMenu->sizeHint();  //最终的大小 prefered size
        int  y = p.y () - size.height();
        m_mainMenu->exec( QPoint( p.x() , y ) );
    }
}
void WeChat::slot_dealMenu(QAction *action)
{
    if( action->text() == "添加好友")
    {
        qDebug()<< "添加好友";
        Q_EMIT SIG_addFriend();
    }else if( action->text() == "创建房间" )
    {
        qDebug()<< "创建房间";
        Q_EMIT SIG_createRoom();
    }else if( action->text() == "加入房间" )
    {
        qDebug()<< "加入房间";
        Q_EMIT SIG_joinRoom();
    }
}
void WeChat::slot_dealStyleMenu(QAction* action)
{
    QString style ;
    if( action->text() == "蓝色妖姬" )
    {
        style = "blue";
    }else if(action->text() == "深色诱惑")
    {
        style = "black";
    }else if(action->text() == "绿色原谅")
    {
        style = "green";
    }else if(action->text() == "金色传说")
    {
        style = "yellow";
    }else if(action->text() == "默认灰色")
    {
        style = "gray";
    }
    Q_EMIT SIG_ChangeStyle(style);
}

void WeChat::on_pb_close_clicked()
{
    if( QMessageBox::question( this , "退出提示","确定退出?" ) == QMessageBox::Yes )
    {
        this->close();
        Q_EMIT SIG_close();
    }
}

void WeChat::on_pb_min_clicked()
{
    this->slot_showMin();
}

//点击换肤
void WeChat::on_pb_style_clicked()
{
    m_styleMenu->exec( QCursor::pos() );
}
