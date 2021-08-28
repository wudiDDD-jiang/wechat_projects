#include "chatdialog.h"
#include "ui_chatdialog.h"
#include"qDebug"

ChatDialog::ChatDialog(QWidget *parent) :
    /*QDialog*/CustomMoveDialog(parent),
    ui(new Ui::ChatDialog) , m_id(0)
{
    ui->setupUi(this);

    m_fileLayout = new QVBoxLayout;
    m_fileLayout->setContentsMargins( 3,5,3,0 );
    m_fileLayout->setSpacing(3);

    ui->wdg_fileList->setLayout( m_fileLayout );

    m_font = QFont("微软雅黑",12);

    m_bq = new bqForm ;
    m_bq->hide();
    connect(m_bq , SIGNAL(SIG_BQID(qint32)) ,this , SLOT(slot_InsertBQToEdit(qint32)) );
}

ChatDialog::~ChatDialog()
{
    delete ui;
    if( m_bq )
    {
        m_bq->hide();
        delete m_bq;
    }
}

//设置成员
void ChatDialog::setInfo(int id, QString name)
{
    //成员
    m_id = id;
    m_name = name ;
    //ui
    ui->lb_tittle->setText( QString("与[ %1 ]的聊天").arg(m_name));
}

#include<QTime>
// 发送聊天
void ChatDialog::on_pb_send_clicked()
{
    QString content = ui->te_chat->toPlainText();
    if( content.isEmpty() ) return;

    ui->te_chat->setFont( m_font );
    content = ui->te_chat->toHtml();
    ui->tb_chat->setTextColor(Qt::black);
    ui->tb_chat->append( QString("[ 我 ] %1").arg(QTime::currentTime().toString("hh:mm:ss")) );
    ui->tb_chat->append( content );

    ui->te_chat->clear();

    Q_EMIT SIG_SendChatMsg( m_id , content );

}
//接收聊天内容
void ChatDialog::slot_recvChatMsg(QString content)
{
    ui->tb_chat->setTextColor(Qt::blue);
    ui->tb_chat->append( QString("[ %1 ] %2")
                         .arg(m_name).arg(QTime::currentTime().toString("hh:mm:ss")) );
    ui->tb_chat->append( content );
}

//显示离线
void ChatDialog::slot_showOffline()
{
    ui->tb_chat->setTextColor(Qt::blue);
    ui->tb_chat->append( QString("用户[ %1 ]已离线").arg(m_name) );
}

void ChatDialog::on_pb_min_clicked()
{
    this->slot_showMin();
}

void ChatDialog::on_pb_max_clicked()
{
    this->slot_showMax();
}

void ChatDialog::on_pb_close_clicked()
{
    this->close();
}

#include<QFileDialog>
void ChatDialog::on_pb_tool2_clicked()
{
    QString strPath = QFileDialog::getOpenFileName(this, "打开" );
    QString tmpPath = strPath;
    if( ! ( tmpPath.remove(" ").isEmpty() ) )
    {
        qDebug()<< strPath;
        Q_EMIT SIG_SendFile( m_id , strPath );
    }
}

//隐藏右侧显示
void ChatDialog::on_pb_hide_clicked()
{
    if( ui->wdg_right->isVisible() )
        ui->wdg_right->setVisible(false);
    else
        ui->wdg_right->setVisible(true);
}

void ChatDialog::slot_addFileItem(FileItem *item)
{
    qDebug()<<__func__;
    m_fileLayout->addWidget( item );
}

//显示表情
void ChatDialog::on_pb_tool3_clicked()
{
    QPoint pp = QCursor::pos() ;

    m_bq->move(pp.x(), pp.y()-220);

    m_bq->show();
    ui->pb_tool3->update();
}

void ChatDialog::slot_InsertBQToEdit(qint32 id)
{
    QString str = QString("%1").arg(id) ;

    QString imgPath = QString(":/bq/%1.png").arg(str,3,QChar('0'));
    imgPath = QString("<img src=\"%1\" width=\"30\" height=\"30\" />").arg(imgPath);

    ui->pb_tool3->update();
    ui->te_chat->insertHtml(imgPath);//inserthtml
}

#include<QFontDialog>
//设置字体
void ChatDialog::on_pb_tool1_clicked()
{
    bool ok =false;
   QFont font = QFontDialog::getFont(   &ok  , QFont( "微软雅黑", 12 ), this );
      if ( ok ) {
          m_font = font;
          // font被设置为用户选择的字体
          ui->te_chat->setCurrentFont(font);
          ui->te_chat->setFocus();
      } else {
          // 用户取消这个对话框，font被设置为初始值，在这里就是"微软雅黑" , 12
      }
}
