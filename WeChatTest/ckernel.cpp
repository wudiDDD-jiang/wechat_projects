#include "ckernel.h"
#include"qDebug"
#include<QApplication>
#include<QInputDialog>
#include<QMessageBox>
#include<QCryptographicHash>
#include "ui_useritem.h"
#include "ui_roomdialog.h"
#define MD5_KEY 1234


static QByteArray GetMD5( QString val)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    QString key = QString("%1_%2").arg(val).arg( MD5_KEY );

    hash.addData(  key.toLocal8Bit() );
    QByteArray bt =  hash.result();

    return bt.toHex();  // AD1234F....   32位  MD5
}


CKernel::CKernel(QObject *parent) : QObject(parent)
  ,m_iconID(0),m_id(0),m_roomid(0)
{
    qDebug()<<__func__;

    std::string strIP = DEF_SERVER_IP;
    m_serverIP = QString::fromStdString( strIP );

    InitConfig();
    setNetPackMap();

    m_weChatDlg = new WeChat;
//    m_weChatDlg->show();

    connect(m_weChatDlg , SIGNAL(SIG_close()) , this, SLOT(slot_quit())  );
    connect(m_weChatDlg ,SIGNAL( SIG_addFriend()) , this , SLOT(slot_addFriend()) );
    connect(m_weChatDlg ,SIGNAL( SIG_createRoom()) , this , SLOT(slot_createRoom()) );
    connect(m_weChatDlg ,SIGNAL( SIG_joinRoom()) , this , SLOT(slot_joinRoom()) );
    connect(m_weChatDlg ,SIGNAL( SIG_ChangeStyle(QString)) , this , SLOT(slot_ChangeStyle(QString)) );

    m_tcpClient = new QMyTcpClient;
    connect( m_tcpClient , SIGNAL(SIG_ReadyData(char*,int)) ,
             this, SLOT(slot_ReadyData(char*,int)) );
    connect( m_tcpClient , SIGNAL(SIG_disConnect()) ,
             this, SLOT(slot_disConnect()) );

    m_heartTimer = new QTimer;
    connect( m_heartTimer , SIGNAL(timeout()) , this ,SLOT(slot_heartConnect()) );

    m_tcpClient->setIpAndPort( (char*) m_serverIP.toStdString().c_str() );

    m_loginDlg = new LoginDialog;
    connect(m_loginDlg , SIGNAL(SIG_LoginCommit(QString,QString)) ,
            this , SLOT( slot_LoginCommit(QString,QString))  );
    connect(m_loginDlg , SIGNAL(SIG_RegisterCommit(QString,QString)) ,
            this , SLOT( slot_RegisterCommit(QString,QString))  );

    m_loginDlg->showNormal();

    m_roomdlg = new RoomDialog;
    connect( m_roomdlg , SIGNAL(SIG_quitRoom()) ,this , SLOT(slot_leaveRoom()) );

    m_pVideoRead = new Video_Read;
    connect( m_roomdlg , SIGNAL(SIG_openVideoDev())
             ,m_pVideoRead , SLOT( slot_openVideo())  );
    connect( m_roomdlg , SIGNAL(SIG_closeVideoDev())
            ,m_pVideoRead , SLOT( slot_closeVideo()) );

    connect( m_pVideoRead , SIGNAL( SIG_sendVideoFrame(QImage&)),
             this , SLOT( slot_sendVideoFrame(QImage&))   );

    connect( m_roomdlg , SIGNAL(SIG_setMoji(int))
             ,m_pVideoRead , SLOT(slot_setMoji(int)) );

    m_pAudioRead = new Audio_Read;
    connect( m_pAudioRead , SIGNAL(sig_net_tx_frame(QByteArray)) ,
             this , SLOT( slot_sendAudioFrame(QByteArray) ) );

    connect( m_roomdlg , SIGNAL(SIG_openAudioDev()) ,
             m_pAudioRead , SLOT( ResumeAudio())  );

    connect( m_roomdlg , SIGNAL(SIG_closeAudioDev()) ,
             m_pAudioRead , SLOT( PauseAudio())  );

    m_SystemTrayIcon = new QSystemTrayIcon(this);
    m_SystemTrayIcon->setIcon(QIcon(":/images/message.png"));
    m_SystemTrayIcon->setToolTip(tr("科林明伦"));


    m_SystemTrayIcon->setContextMenu(m_weChatDlg->m_mainMenu);
    connect(m_SystemTrayIcon, &QSystemTrayIcon::activated, this, &CKernel::activeTray,Qt::UniqueConnection);//点击托盘，执行相应的动作

    //文件传输进度connect
    connect( this ,SIGNAL(SIG_updateFileProcess(QString,qint64))
             , this , SLOT( slot_updateFileProcess(QString,qint64)) );

    m_uploadWorker = new UploadWorker;
    m_uploadThread = new QThread;
    connect( this , SIGNAL(SIG_dealUploadRs(char*,int))
             , m_uploadWorker , SLOT(slot_uploadThread(char*,int)) );
    m_uploadWorker->moveToThread( m_uploadThread );

    m_uploadThread->start();

}

//单例
CKernel *CKernel::GetInstance()
{
    static CKernel kernel;
    return &kernel;
}

void CKernel::DestroyInstance()
{
    qDebug()<<__func__;
    if( m_heartTimer )
    {
        m_heartTimer->stop();
        delete m_heartTimer;m_heartTimer = NULL;
    }
    if( m_pVideoRead )
    {
        m_pVideoRead->slot_closeVideo();
        delete m_pVideoRead ; m_pVideoRead = NULL;
    }

    if(m_loginDlg )
    {
        delete m_loginDlg; m_loginDlg = NULL;
    }

    for( auto ite = m_mapIDToChatdlg.begin() ;ite !=  m_mapIDToChatdlg.end() ;++ite )
    {
        ChatDialog * chat = *ite;
        chat->close();
        delete *ite;
        *ite = NULL;
    }
    m_mapIDToChatdlg.clear();

    if( m_pAudioRead )
    {
        m_pAudioRead->PauseAudio();
        delete m_pAudioRead; m_pAudioRead = NULL;
    }
    for( auto ite = m_mapIDToAudioWrite.begin() ;ite !=  m_mapIDToAudioWrite.end() ;++ite )
    {
        delete *ite;
        *ite = NULL;
    }
    m_mapIDToAudioWrite.clear();

    if( m_roomid != 0 )
    {
        this->slot_leaveRoom();
        delete m_roomdlg; m_roomdlg = NULL;
    }
    if( m_weChatDlg )
    {
        m_weChatDlg->close();
        delete m_weChatDlg; m_weChatDlg = NULL;
    }
    if( m_SystemTrayIcon )
    {
        m_SystemTrayIcon->hide();
    }
    if( m_uploadWorker )
    {
        delete m_uploadWorker;
        m_uploadWorker = NULL;
    }
    if( m_uploadThread )
    {
        m_uploadThread->quit();
        m_uploadThread->wait(100);
        if( m_uploadThread->isRunning() )
        {
            m_uploadThread->terminate(); m_uploadThread->wait(100);
        }
        delete m_uploadThread; m_uploadThread = NULL;
    }
    if( m_tcpClient )
    {
        delete m_tcpClient; m_tcpClient = NULL;
    }
}

#include<QSettings>
#include<QCoreApplication>
#include<QFileInfo>
// 加载配置文件  QSettings
void CKernel::InitConfig()
{
///////////////////
//[net]
//ip="192.168.5.31"
///////////////////
    //获取路径 exe路径 E:\2020fall\0401\build-MyQQ-Desktop_Qt_5_6_2_MinGW_32bit-Release\release   拼接成配置文件路径
    QString path = QCoreApplication::applicationDirPath() + "/config.ini";
    QFileInfo info(path);
    if( info.exists() )
    {// 如果存在 读取设置 ip

        QSettings setting( path , QSettings::IniFormat , NULL ); //相当于打开配置文件 存在读取不存在创建
        setting.beginGroup("net");
        QVariant var =  setting.value("ip");
        QString ip = var.toString();
        if( !ip.isEmpty() )  m_serverIP = ip;
        setting.endGroup();
        qDebug()<< m_serverIP;

        QString color = "default";
        setting.beginGroup("theme");
        var =  setting.value("color");
        QString tmpColor = var.toString();
        if( !tmpColor.isEmpty() )  color = tmpColor;
        setting.endGroup();

        setQss( color );
    }else
    {
    // 如果不存在创建 并写默认
        QSettings setting( path , QSettings::IniFormat , NULL ); //相当于打开配置文件 存在读取不存在创建
        setting.beginGroup("net");
        setting.setValue("ip", m_serverIP);  //将默认写入文件
        setting.endGroup();
        setting.beginGroup("theme");
        setting.setValue("color", "default");  //将默认写入文件
        setting.endGroup();

        setQss("default");
    }
}

void CKernel::setQss(QString qss)
{
    QString path = QString(":/qss/%1.css")
            .arg(qss);

    qDebug()<< path;
    QFile file( path );
    if( file.open(QIODevice::ReadOnly) )
    {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}
#define NetPackFunMap(a) m_NetPackFunMap[ a - DEF_PACK_BASE]
//协议映射表
void CKernel::setNetPackMap()
{
    memset( m_NetPackFunMap , 0 , sizeof(PFUN)*DEF_PACK_COUNT );
    NetPackFunMap( DEF_PACK_LOGIN_RS        ) = &CKernel::slot_dealLoginRs;
    NetPackFunMap( DEF_PACK_REGISTER_RS     ) = &CKernel::slot_dealRegisterRs;
    NetPackFunMap( DEF_PACK_FORCE_OFFLINE   ) = &CKernel::slot_dealForceOffline;
    NetPackFunMap( DEF_PACK_ADD_FRIEND_RQ   ) = &CKernel::slot_dealAddFriendRq;
    NetPackFunMap( DEF_PACK_ADD_FRIEND_RS   ) = &CKernel::slot_dealAddFriendRs;
    NetPackFunMap( DEF_PACK_FRIEND_INFO     ) = &CKernel::slot_dealFriendInfo;
    NetPackFunMap( DEF_PACK_OFFLINRE_RS     ) = &CKernel::slot_dealOfflineRs;
    NetPackFunMap( DEF_PACK_CHAT_RQ         ) = &CKernel::slot_dealChatRq;
    NetPackFunMap( DEF_PACK_CHAT_RS         ) = &CKernel::slot_dealChatRs;
    NetPackFunMap( DEF_PACK_CREATEROOM_RS   ) = &CKernel::slot_dealCreateRoomRs;
    NetPackFunMap( DEF_PACK_JOINROOM_RS     ) = &CKernel::slot_dealJoinRoomRs;
    NetPackFunMap( DEF_PACK_ROOM_MEMBER     ) = &CKernel::slot_dealRoomMember;
    NetPackFunMap( DEF_PACK_LEAVEROOM_RS    ) = &CKernel::slot_dealLeaveRoomRs;
    NetPackFunMap( DEF_PACK_VIDEO_FRAME     ) = &CKernel::slot_dealVideoFrame;
    NetPackFunMap( DEF_PACK_AUDIO_FRAME     ) = &CKernel::slot_dealAudioFrame;
    NetPackFunMap( DEF_PACK_UPLOAD_RS       ) = &CKernel::slot_dealUploadRs;
    NetPackFunMap( DEF_PACK_UPLOAD_RQ       ) = &CKernel::slot_dealUploadRq;
    NetPackFunMap( DEF_PACK_FILEBLOCK_RQ    ) = &CKernel::slot_dealFileBlockRq;
}

//与服务器断连提醒
void CKernel::slot_disConnect()
{
    QMessageBox::information( m_weChatDlg ,"警告" ,"与服务器失去连接,重新登录" );

    m_SystemTrayIcon->hide();
    //恢复初始状态
    if( m_roomid != 0 )
    {
        slot_leaveRoom();
    }
    for( auto ite = m_mapIDToChatdlg.begin();ite !=m_mapIDToChatdlg.end();++ite )
    {
        (*ite)->hide();
    }
    m_weChatDlg->hide();

    m_loginDlg->showNormal();
}
void CKernel::activeTray(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Context:
        m_weChatDlg->on_pb_menu_clicked();
        break;
    case QSystemTrayIcon::DoubleClick:
        m_weChatDlg->showNormal();
        break;
//    case QSystemTrayIcon::Trigger:
//        showMessage();
//        break;
    }
}
//心跳
void CKernel::slot_heartConnect()
{
    //发送心跳包
    STRU_HEART rq;
    rq.m_nUserId = m_id;
    if( m_tcpClient->IsConnected() )
        m_tcpClient->SendData( (char*)&rq , sizeof(rq) );
}

//网络包处理
void CKernel::slot_ReadyData(char *buf, int nlen)
{
    int type = *(int*) buf;
    if( type >= DEF_PACK_BASE && type < DEF_PACK_BASE + DEF_PACK_COUNT ) {
        if( type == DEF_PACK_UPLOAD_RS )
        {
            Q_EMIT SIG_dealUploadRs( buf , nlen );
            return;
        }

        PFUN p = m_NetPackFunMap[ type - DEF_PACK_BASE ];
        if( p != NULL )
        {
            (this->*p)( buf , nlen );
        }
    }

    delete[] buf;
}



//处理登录回复
void CKernel::slot_dealLoginRs(char *buf, int nlen)
{
    STRU_LOGIN_RS * rs = (STRU_LOGIN_RS *)buf;
    switch( rs->m_lResult )
    {
    case userid_no_exist:
        QMessageBox::about( m_loginDlg , "提示" , "登录失败, 用户不存在");
        break;
    case password_error:
        QMessageBox::about( m_loginDlg , "提示" , "登录失败, 密码错误");
        break;
    case login_sucess:
        // QMessageBox::about( m_loginDlg , "提示" , "登录成功");
        // id 赋值
        m_id = rs->m_UserID;
        // ui 跳转
        m_loginDlg->hide();
        m_weChatDlg ->showNormal();
//        m_heartTimer->start( 2000 );
        m_SystemTrayIcon->show();
        break;
    default:break;
    }
}
//注册回复
void CKernel::slot_dealRegisterRs(char *buf, int nlen)
{
    STRU_REGISTER_RS * rs = (STRU_REGISTER_RS *)buf;
    switch( rs->m_lResult )
    {
    case userid_is_exist:
        QMessageBox::about( m_loginDlg , "提示" , "注册失败, 用户已存在");
        break;
    case register_sucess:
        QMessageBox::about( m_loginDlg , "提示" , "注册成功");
        break;
    default:break;
    }
}
//强制下线
void CKernel::slot_dealForceOffline(char *buf, int nlen)
{
    STRU_FORCE_OFFLINE * rs = (STRU_FORCE_OFFLINE *)buf;
    if(rs->m_UserID == m_id )
    {
        QMessageBox::about( m_weChatDlg , "提示","异地登录, 强制下线");

        //退出程序 -- todo
        DestroyInstance();
        //exit(0);
    }
}

//处理添加好友请求
void CKernel::slot_dealAddFriendRq(char *buf, int nlen)
{
    STRU_ADD_FRIEND_RQ * rq = (STRU_ADD_FRIEND_RQ *)buf;
    STRU_ADD_FRIEND_RS rs;
    strcpy_s( rs.szAddFriendName , MAX_SIZE , rq->m_szAddFriendName );
    rs.m_friendID = m_id;
    rs.m_userID = rq->m_userID ;

    if( QMessageBox::question( m_weChatDlg , "好友请求" , QString( "用户[%1] 请求添加你为好友").arg(rq->m_szUserName) )
            == QMessageBox::Yes )
    {
        rs.m_result = add_success;
    }else
    {
        rs.m_result = user_refused;
    }
    m_tcpClient -> SendData( (char*)&rs , sizeof(rs));

}
////添加好友结果
//#define no_this_user    0
//#define user_refused    1
//#define user_is_offline 2
//#define add_success     3
//处理添加好友恢复
void CKernel::slot_dealAddFriendRs(char *buf, int nlen)
{
    STRU_ADD_FRIEND_RS* rs = (STRU_ADD_FRIEND_RS *)buf;
    switch( rs->m_result )
    {
    case no_this_user:
        QMessageBox::about(m_weChatDlg , "提示","没有此用户");
        break;
    case user_refused:
        QMessageBox::about(m_weChatDlg , "提示","用户拒绝添加");
        break;
    case user_is_offline:
        QMessageBox::about(m_weChatDlg , "提示","用户离线");
        break;
    case add_success:
        QMessageBox::about(m_weChatDlg , "提示", QString("成功添加[%1]为好友").arg(rs->szAddFriendName));
        break;
    }
}

//好友列表
void CKernel::slot_dealFriendInfo(char *buf, int nlen)
{
    STRU_FRIEND_INFO * friendrq = (STRU_FRIEND_INFO *)buf;

    if( friendrq->m_userID == m_id )
    {
        m_UserName = friendrq->m_szName;

        m_weChatDlg->slot_setInfo( m_id ,friendrq->m_szName, QString(":/tx/%1.png").arg(friendrq->m_iconID) );
        return;
    }
    if( m_mapIDToUserItem .find(friendrq->m_userID) == m_mapIDToUserItem.end() )
    {//没找到
        UserItem * item = new UserItem;
        item->setInfo( friendrq->m_userID , friendrq->m_state ,friendrq->m_szName ,
                       QString(":/tx/%1.png").arg(friendrq->m_iconID) );

        m_weChatDlg->slot_addUser(item);
        connect( item , SIGNAL(SIG_ItemClicked()) ,
                 this, SLOT( slot_UserItemClicked()) );

        m_mapIDToUserItem[ friendrq->m_userID ] = item;

        //创建聊天的窗口
        ChatDialog * chat = new ChatDialog;
        chat->setInfo( friendrq->m_userID  ,friendrq->m_szName);
        connect( chat , SIGNAL(SIG_SendChatMsg(int,QString)) ,
                 this , SLOT( slot_SendChatRq(int,QString) ) );
        connect( chat , SIGNAL( SIG_SendFile(int , QString) ) ,
                 this , SLOT( slot_SendFileRq(int , QString) ) );

        m_mapIDToChatdlg[ friendrq->m_userID ] = chat;

    }else
    {//找到
        UserItem * item = m_mapIDToUserItem[ friendrq->m_userID ];

        if( item->m_state == 0 )
        {
            Notify *nf = new Notify;
            nf->setMsg( item->m_name , item->m_id , QString("[ %1 ]上线了").arg(item->m_name));
            nf->showAsQQ();
        }
        item->setInfo( friendrq->m_userID , friendrq->m_state ,friendrq->m_szName ,
                       QString(":/tx/%1.png").arg(friendrq->m_iconID) );
    }
}

//用户离线设置
void CKernel::slot_dealOfflineRs(char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_OFFLINE_RS *rs = (STRU_OFFLINE_RS *)buf;
    if( m_mapIDToUserItem.find( rs->m_userID ) != m_mapIDToUserItem.end() )
    {

        UserItem* item =   m_mapIDToUserItem[rs->m_userID]  ;
        item->setInfo( item->m_id , 0 ,item->m_name, item->m_icon );

        //发送离线通知
        Notify *nf = new Notify;
        nf->setMsg( item->m_name , item->m_id , QString("[ %1 ]下线了").arg(item->m_name));
        nf->showAsQQ();

    }
}

//处理聊天请求
void CKernel::slot_dealChatRq(char *buf, int nlen)
{
    STRU_CHAT_RQ *rq = (STRU_CHAT_RQ *)buf;

    // 对方用户 rq->m_userID
    if( m_mapIDToChatdlg.find( rq->m_userID) != m_mapIDToChatdlg.end() )
    {
        ChatDialog*chat =  m_mapIDToChatdlg[rq->m_userID];
        std::string strContent = rq->m_ChatContent;

        chat->slot_recvChatMsg( QString::fromStdString( strContent )  );
        chat->hide();
        chat->showNormal();
        //发送离线通知
        Notify *nf = new Notify;
        nf->setMsg( chat->m_name , chat->m_id , QString("[ %1 ]\n发来一则消息").arg(chat->m_name));
        nf->showAsQQ();
    }
}
//处理聊天回复
void CKernel::slot_dealChatRs(char *buf, int nlen)
{
    STRU_CHAT_RS *rs = (STRU_CHAT_RS *)buf;

    // 对方用户 rs->m_friendID
    if( rs->m_result == user_is_offline )
    {
        if( m_mapIDToChatdlg.find( rs->m_friendID ) != m_mapIDToChatdlg.end() )
        {
            ChatDialog*chat =  m_mapIDToChatdlg[rs->m_friendID];
            chat->slot_showOffline();
        }
    }
}

//处理创建房间回复
void CKernel::slot_dealCreateRoomRs(char *buf, int nlen)
{
    STRU_CREATEROOM_RS *rs = (STRU_CREATEROOM_RS *)buf;
    //rs->m_RoomId

    m_roomid = rs->m_RoomId;

    //ui
    m_roomdlg->slot_setInfo( rs->m_RoomId );
    m_roomdlg->showNormal();

    //添加对自己video的显示
    if( m_mapIDToVideoItem.find( m_id ) == m_mapIDToVideoItem.end() )
    {
        VideoItem* item = new VideoItem;
        item->setInfo( m_id , m_UserName );
        connect( item , SIGNAL(SIG_itemClicked(int)) ,
                 this , SLOT( slot_videoItemClicked(int) ) );
        //connect( item , &VideoItem::SIG_itemClicked , this , SLOT() );
        m_roomdlg->slot_addVideoItem( item );
        m_mapIDToVideoItem[ m_id ] = item;
    }
}

//处理加入房间回复
void CKernel::slot_dealJoinRoomRs(char *buf, int nlen)
{
    STRU_JOINROOM_RS *rs = (STRU_JOINROOM_RS *)buf;

    if( rs->m_lResult )
    {
        m_roomid = rs->m_RoomID;

        //ui
        m_roomdlg->slot_setInfo( rs->m_RoomID );

        m_roomdlg->showNormal();

        //添加对自己video的显示
        if( m_mapIDToVideoItem.find( m_id ) == m_mapIDToVideoItem.end() )
        {
            VideoItem* item = new VideoItem;
            item->setInfo( m_id , m_UserName );
            connect( item , SIGNAL(SIG_itemClicked(int)) ,
                     this , SLOT( slot_videoItemClicked(int) ) );
            //connect( item , &VideoItem::SIG_itemClicked , this , SLOT() );
            m_roomdlg->slot_addVideoItem( item );
            m_mapIDToVideoItem[ m_id ] = item;
        }

        // 音频和视频

    }else
    {
        QMessageBox::about( m_weChatDlg , "提示" , "加入房间失败");
    }
}
//房间内好友列表
void CKernel::slot_dealRoomMember(char *buf, int nlen)
{
    //拆包
    STRU_ROOM_MEMBER_RQ * rq = (STRU_ROOM_MEMBER_RQ *)buf;
    //rq->m_UserID;
    //rq->m_szUser;
    //加video显示
    if( m_mapIDToVideoItem.find( rq->m_UserID ) == m_mapIDToVideoItem.end() )
    {
        VideoItem* item = new VideoItem;
        item->setInfo(rq->m_UserID , rq->m_szUser );
        connect( item , SIGNAL(SIG_itemClicked(int)) ,
                 this , SLOT( slot_videoItemClicked(int) ) );
        //connect( item , &VideoItem::SIG_itemClicked , this , SLOT() );
        m_roomdlg->slot_addVideoItem( item );
        m_mapIDToVideoItem[ rq->m_UserID ] = item;


    }
    //音频
    if( m_mapIDToAudioWrite.find( rq->m_UserID ) == m_mapIDToAudioWrite.end() )
    {
        Audio_Write * item = new Audio_Write;
        m_mapIDToAudioWrite[rq->m_UserID ] = item;
    }
}
//房间内某人离开
void CKernel::slot_dealLeaveRoomRs(char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_LEAVEROOM_RS * rs = (STRU_LEAVEROOM_RS *)buf;

    //map中有这个人就删掉 显示也回收
    auto ite = m_mapIDToVideoItem.find( rs->m_UserID );
    if( ite != m_mapIDToVideoItem.end() )
    {
        VideoItem * item = *ite;
        m_roomdlg->slot_removeVideoItem( item );

        m_roomdlg->repaint();

        m_mapIDToVideoItem.erase( ite );

        delete item;
        item = NULL;
    }
    // 这个人 音频 视频
    auto iteAudio = m_mapIDToAudioWrite.find( rs->m_UserID );
    if( iteAudio != m_mapIDToAudioWrite.end() )
    {
        Audio_Write *item = m_mapIDToAudioWrite[rs->m_UserID];
        m_mapIDToAudioWrite.erase( iteAudio );
        delete item ;
        item = NULL;
    }
}
//解析视频帧
void CKernel::slot_dealVideoFrame(char *buf, int nlen)
{
    // buf --> QImage
    ///视频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray videoFrame;
    char* tmp = buf;
    int type = *(int*)tmp;
    tmp += sizeof(int);
    int userid= *(int*)tmp;
    tmp += sizeof(int);
    int roomid= *(int*)tmp;
    tmp += sizeof(int);

    QByteArray ba( tmp , nlen -12 );
    QImage img;
    img.loadFromData( ba );

    this->slot_refreshImage( userid , img );
}
//解析音频并播放
void CKernel::slot_dealAudioFrame(char *buf, int nlen)
{
    /// 音频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray audioFrame;
    char* tmp = buf;
    int type = *(int*) tmp ;
    tmp += sizeof(int);
    int userId= *(int*) tmp ;
    tmp += sizeof(int);
    int roomId= *(int*) tmp ;
    tmp += sizeof(int);

    if( m_mapIDToAudioWrite.find( userId ) != m_mapIDToAudioWrite.end())
    {
        QByteArray byte( tmp , nlen -12 );
        Audio_Write *item = m_mapIDToAudioWrite[userId];
        item->slot_net_rx( byte );
    }
}

//处理上传回复
void CKernel::slot_dealUploadRs(char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_UPLOAD_RS * rs = (STRU_UPLOAD_RS *)buf;
    if( rs->m_nResult == 0)
    {
        //打开对方聊天 添加信息
        if( m_mapIDToChatdlg.find( rs->m_friendId) != m_mapIDToChatdlg.end() )
        {
            ChatDialog * chat = m_mapIDToChatdlg[rs->m_friendId];
            chat->slot_recvChatMsg( "拒绝接收文件");
            return;
        }
    }

    std::string strMD5 = rs->m_szFileMD5;
    QString MD5 = QString::fromStdString( strMD5 );
    qDebug()<< "MD5" << MD5;
    if( m_mapFileMD5ToFileInfo.find(MD5)!= m_mapFileMD5ToFileInfo.end() )
    {
        FileInfo * info = m_mapFileMD5ToFileInfo[MD5];
        info->pFile = new QFile(info->filePath);
        if( info->pFile->open(QIODevice::ReadOnly ) )
        {
            while( 1 )
            {
                qDebug()<<"STRU_FILEBLOCK_RQ";
                STRU_FILEBLOCK_RQ rq;
                rq.m_nUserId = m_id;
                rq.m_friendId = rs->m_friendId;

                int res  = info->pFile->read( rq.m_szFileContent ,MAX_CONTENT_LEN );
                strcpy_s( rq.m_szFileMD5 , MAX_PATH , rs->m_szFileMD5 );


                info->filePos += res ;
                rq.m_nBlockLen = res;

                // 抛出信号  更新进度
                Q_EMIT SIG_updateFileProcess( info->fileMD5 , info->filePos );

                m_tcpClient->SendData( (char*)&rq , sizeof(rq) );

                if( info->filePos >= info->fileSize )
                {
                    info->pFile->close();
                    //去除该节点
                    m_mapFileMD5ToFileInfo.remove( MD5 );
                    delete info;
                    info = NULL;

                    //QMessageBox::about( NULL , "提示","发送完成" );

                    break;
                }
            }

        }else
        {
            qDebug() << "open file fail";
        }
    }
}

//处理上传
void CKernel::slot_dealUploadRq(char *buf, int nlen)
{
    //chat 对应的控件显示 下载项

    STRU_UPLOAD_RQ * rq = (STRU_UPLOAD_RQ * ) buf;

    FileInfo* info = new FileInfo;
    info->fileMD5 = QString::fromStdString( rq->m_szFileMD5 );
    info->fileName = QString::fromStdString( rq->m_szFileName );
    info->filePos = 0;
    info->fileSize = rq->m_nFileSize;
    info->pFile = NULL;
    info->userID = rq->m_UserId;
    info->friendID = rq->m_friendId;

    m_mapFileMD5ToFileInfo[ info->fileMD5  ] = info ;

    FileItem* item = new FileItem;
    item->slot_setInfo( info->fileMD5 , info->fileName , "" , info->fileSize);
    item->slot_setState( acceptOrReject );

    connect( item , SIGNAL(SIG_fileAccept()) , this , SLOT(slot_fileAccept() ) );
    connect( item , SIGNAL(SIG_fileReject()) , this , SLOT(slot_fileReject() ) );

    m_mapFileMD5ToFileItem[item->m_fileMD5] = item;

    if( m_mapIDToChatdlg.find( rq->m_UserId ) != m_mapIDToChatdlg.end() )
    {
        ChatDialog * chat = m_mapIDToChatdlg[rq->m_UserId];
        chat->slot_addFileItem( item );
        chat->showNormal();
    }

}

//处理发送的文件块
void CKernel::slot_dealFileBlockRq(char *buf, int nlen)
{
    STRU_FILEBLOCK_RQ *rq = (STRU_FILEBLOCK_RQ *)buf;
    QString strMD5 = QString::fromStdString( rq->m_szFileMD5 );

    if( m_mapFileMD5ToFileInfo.find(strMD5 ) != m_mapFileMD5ToFileInfo.end()  )
    {
        FileInfo * info = m_mapFileMD5ToFileInfo[ strMD5 ];
        int res = info->pFile->write( rq->m_szFileContent , rq->m_nBlockLen );

        info->filePos += res;

        Q_EMIT SIG_updateFileProcess(strMD5 , info->filePos );

        if( info->filePos >= info->fileSize )
        {
            info->pFile->close();
            delete info->pFile;
            m_mapFileMD5ToFileInfo.remove( strMD5 );
        }
    }
}

void CKernel::slot_quit()
{
    qDebug()<<__func__;
    //发送离线消息
    STRU_OFFLINE_RQ rq;
    rq.m_userID = m_id;
    m_tcpClient->SendData( (char*)&rq , sizeof(rq));

    DestroyInstance();
}

//提交登录
void CKernel::slot_LoginCommit(QString name, QString password)
{
    STRU_LOGIN_RQ rq;
    //Unicode 中文兼容
    std::string strName = name.toStdString();
    const char* bufName = strName.c_str();
    strcpy_s( rq.m_szUser , MAX_SIZE , bufName );

    QByteArray btPassord = GetMD5 ( password );
    memcpy( rq.m_szPassword , btPassord.data() , btPassord.size() );

    m_UserName = name;
    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}
//提交注册
void CKernel::slot_RegisterCommit(QString name, QString password)
{
    STRU_REGISTER_RQ rq;
    //Unicode 中文兼容
    std::string strName = name.toStdString();
    const char* bufName = strName.c_str();
    strcpy_s( rq.m_szUser , MAX_SIZE , bufName );

    QByteArray btPassord = GetMD5 ( password );
    memcpy( rq.m_szPassword , btPassord.data() , btPassord.size() );

    qDebug()<< rq.m_szPassword;
    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}

void CKernel::slot_addFriend()
{
    QString strName = QInputDialog::getText( m_weChatDlg , "添加好友" , "输入好友名字");
    QRegExp reg("[a-zA-Z0-9]{1,10}");
    bool res = reg.exactMatch( strName );
    if( !res )
    {
        //发送添加好友信号
        QMessageBox::about(m_weChatDlg, "提示" , "用户名非法\n输入10位以内的数字或字母");
        return;
    }
    if( m_UserName == strName )
    {
        QMessageBox::about(m_weChatDlg , "提示","不能添加自己");
        return;
    }
    //已经是好友不可以
    for( auto ite = m_mapIDToUserItem.begin() ; ite != m_mapIDToUserItem.end() ; ++ite)
    {
        //todo
         UserItem *item = *ite;
         if( item ->m_name == strName )
         {
             QMessageBox::about(m_weChatDlg , "提示","已经是好友,不能添加");
             return;
         }
    }

    STRU_ADD_FRIEND_RQ rq;
    std::string  strAddName = strName.toStdString();
    strcpy_s( rq.m_szAddFriendName , MAX_SIZE , strAddName.c_str() );

    rq.m_userID = m_id;
    std::string  strUserName =  m_UserName.toStdString();
    strcpy_s( rq.m_szUserName , MAX_SIZE, strUserName.c_str() );

    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}
//双击用户响应
void CKernel::slot_UserItemClicked()
{
    qDebug()<<__func__;

    UserItem * item = (UserItem *) QObject::sender();
    //聊天窗口显示

    if(  m_mapIDToChatdlg.find( item->m_id ) != m_mapIDToChatdlg.end()  )
    {
         ChatDialog *chat = m_mapIDToChatdlg[ item->m_id];
         chat->hide();
         chat->showNormal();
    }
}
//创建房间
void CKernel::slot_createRoom()
{
    if( m_roomid != 0 )
    {
        QMessageBox::about(m_weChatDlg, "提示","已经在房间里");
        return;
    }
    STRU_CREATEROOM_RQ rq;
    rq.m_UserID = m_id;

    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}

//
void CKernel::slot_joinRoom()
{
    //qDebug()<< "加入房间";
    if( m_roomid != 0 )
    {
        QMessageBox::about(m_weChatDlg, "提示","已经在房间里");
        return;
    }
    QString strRoom = QInputDialog::getText( m_weChatDlg , "加入房间" , "输入房间号");
    QRegExp reg("[0-9]{1,10}");
    bool res = reg.exactMatch( strRoom );
    if( !res )
    {
        QMessageBox::about(m_weChatDlg, "提示" , "房间号非法\n输入8位以内的数字");
        return;
    }
    int id = strRoom.toInt();

    STRU_JOINROOM_RQ rq;
    rq.m_RoomID = id;
    rq.m_UserID = m_id;

    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}
//离开房间
void CKernel::slot_leaveRoom()
{
    STRU_LEAVEROOM_RQ rq;
    rq.m_nUserId = m_id;
    rq.m_RoomId = m_roomid;

    m_tcpClient->SendData( (char*)&rq , sizeof(rq));

    //回收显示 从控件中移除显示 然后delete
    for( auto ite = m_mapIDToVideoItem.begin(); ite !=m_mapIDToVideoItem.end();++ite)
    {
        VideoItem* item = *ite;
        m_roomdlg->slot_removeVideoItem( item );

        delete item;
        item = NULL;
    }
    m_mapIDToVideoItem.clear();

    //关闭视频 音频
    if( m_pVideoRead )
    {
        m_pVideoRead->slot_closeVideo();
    }
    if( m_pAudioRead )
    {
        m_pAudioRead->PauseAudio();
    }

    //回收所有声音播放的item
    for( auto ite = m_mapIDToAudioWrite.begin(); ite !=m_mapIDToAudioWrite.end();++ite)
    {
        Audio_Write* item = *ite;

        delete item;
        item = NULL;
    }
    m_mapIDToAudioWrite.clear();

    m_roomid = 0;
}

//执行换肤
void CKernel::slot_ChangeStyle(QString style)
{
    //换肤
    setQss( style );
    //写入配置
    QString path = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings setting( path , QSettings::IniFormat , NULL ); //相当于打开配置文件 存在读取不存在创建
    setting.beginGroup("theme");
    setting.setValue("color", style);  //将默认写入文件
    setting.endGroup();
}

//发送聊天请求
void CKernel::slot_SendChatRq(int id, QString content)
{
    STRU_CHAT_RQ rq;
    rq.m_userID = m_id;
    rq.m_friendID = id;

    std::string strContent = content.toStdString();
    strcpy_s(  rq.m_ChatContent , MAX_CONTENT_LEN , strContent.c_str() );

    m_tcpClient->SendData( (char*)&rq , sizeof(rq));
}

//右侧缩略视频被点击
void CKernel::slot_videoItemClicked(int id)
{
    VideoItem* item = (VideoItem*)QObject::sender();
    m_roomdlg->slot_setBigImageID( id ,  item->m_name );
}
#include<QBuffer>
//发送视频帧
void CKernel::slot_sendVideoFrame(QImage &img)
{
    //刷新ui
    slot_refreshImage( m_id , img );

    //将image图片转换成 缓冲区
    QByteArray ba;
    QBuffer qbuf(&ba); // QBuffer 与 QByteArray 字节数组联立联系
    img.save( &qbuf , "JPEG" );  //将图片的数据写入 ba

    //发送到服务器
    //ba.size();
    ///视频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray videoFrame;
    char* buf = new char[ 12 + ba.size() ];
    char* tmp = buf;
    int type = DEF_PACK_VIDEO_FRAME;
    int userId = m_id ;
    int roomId = m_roomid ;

    *(int*)tmp = type;  //*(int*) 按照四个字节去写
    tmp += sizeof(int);
    *(int*)tmp = userId;  //*(int*) 按照四个字节去写
    tmp += sizeof(int);
    *(int*)tmp = roomId;  //*(int*) 按照四个字节去写
    tmp += sizeof(int);

    memcpy( tmp , ba.data() , ba.size() );

    m_tcpClient->SendData(  buf , (12+ ba.size()) );
    delete[] buf;
}

//发送音频帧
void CKernel::slot_sendAudioFrame(QByteArray byte)
{
    /// 音频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray audioFrame;

    int type = DEF_PACK_AUDIO_FRAME;
    int userId = m_id;
    int roomId = m_roomid;

    char *buf = new char[ 12 + byte.size() ];
    char* tmp = buf;

    *(int*)tmp = type;
    tmp += sizeof(int);
    *(int*)tmp = userId;
    tmp += sizeof(int);
    *(int*)tmp = roomId;
    tmp += sizeof(int);

    memcpy( tmp , byte.data() , byte.size() );

    m_tcpClient->SendData( buf, 12+ byte.size() );
    delete[] buf;
}
//刷新界面图片显示
void CKernel::slot_refreshImage(int id, QImage &img)
{
    //更新大图
    if( m_roomdlg->getUi()->wdg_video->m_id == id )
        m_roomdlg->getUi()->wdg_video->slot_setOneImage( img );

    //更新小图
    if( m_mapIDToVideoItem.find( id ) != m_mapIDToVideoItem.end() )
    {
        VideoItem* item = m_mapIDToVideoItem[id];
        item->slot_setOneImage( img );
    }
}
#include<QFile>
#include<QIODevice>
//发送文件流程
void CKernel::slot_SendFileRq(int id, QString strPath)
{
    qDebug()<<__func__;
    QFileInfo fileInfo( strPath );

    QByteArray resMD5 = GetMD5 ( fileInfo.fileName() + QTime::currentTime().toString("hhmmss") );

    std::string strFileName = fileInfo.fileName().toStdString();

    STRU_UPLOAD_RQ rq;
    rq.m_friendId = id;
    rq.m_nFileSize = fileInfo.size();
    memcpy( rq.m_szFileMD5 , resMD5.data() , resMD5.size()  );
    strcpy_s( rq.m_szFileName ,MAX_PATH ,strFileName.c_str() );
    rq.m_UserId = m_id;

    qDebug()<< rq.m_szFileMD5;
    //发送文件头
    m_tcpClient->SendData( (char*)&rq , sizeof(rq));

    std::string strMD5 = rq.m_szFileMD5;
    QString MD5 = QString::fromStdString( strMD5 );

    FileInfo * pInfo = new FileInfo;
    pInfo->fileMD5 = MD5 ;
    pInfo->fileName = fileInfo.fileName();
    pInfo->filePath = strPath;
    pInfo->filePos = 0;
    pInfo->fileSize = rq.m_nFileSize;
    pInfo->pFile = NULL;
    pInfo->userID = m_id;
    pInfo->friendID = id;

    m_mapFileMD5ToFileInfo[ MD5 ] = pInfo;

    //创建文件过程显示
    FileItem * fileItem = new FileItem;
    fileItem->slot_setInfo( pInfo->fileMD5 , pInfo->fileName, pInfo->filePath ,pInfo->fileSize );
    fileItem->slot_setState( waitRs );
    m_mapFileMD5ToFileItem[pInfo->fileMD5] = fileItem;

    ChatDialog * chat = (ChatDialog*)QObject::sender();
    if( chat )
    {
        chat->slot_addFileItem( fileItem );
    }
}

//更新MD5对应的文件传输进度
void CKernel::slot_updateFileProcess(QString fileMD5, qint64 cur)
{
    if(m_mapFileMD5ToFileItem.find(fileMD5) != m_mapFileMD5ToFileItem.end() )
    {
        FileItem * item = m_mapFileMD5ToFileItem[ fileMD5 ];
        item->slot_setFileProcess( cur );
    }
}
//同意文件传输
void CKernel::slot_fileAccept()
{
    FileItem * item = (FileItem*)QObject::sender();

    if( !item) return;

    if( m_mapFileMD5ToFileInfo.find( item->m_fileMD5 ) != m_mapFileMD5ToFileInfo.end() )
    {
        FileInfo* info = m_mapFileMD5ToFileInfo[item->m_fileMD5 ];
        info->filePath = item->m_filePath;

        STRU_UPLOAD_RS rs;
        rs.m_friendId = info->friendID;
        rs.m_nResult = 1;
        strcpy( rs.m_szFileMD5 , info->fileMD5.toStdString().c_str() );
        rs.m_UserId = info->userID;

        info->pFile = new QFile( info->filePath );

        if( !info->pFile->open( QIODevice::WriteOnly ) )
        {
            rs.m_nResult = 0;
        }
        m_tcpClient->SendData( (char*)&rs, sizeof(rs) );
    }
}
//拒绝文件传输
void CKernel::slot_fileReject()
{
    FileItem * item = (FileItem*)QObject::sender();

    if( !item) return;

    if( m_mapFileMD5ToFileInfo.find( item->m_fileMD5 ) != m_mapFileMD5ToFileInfo.end() )
    {
        FileInfo* info = m_mapFileMD5ToFileInfo[item->m_fileMD5 ];
        info->filePath = item->m_filePath;

        STRU_UPLOAD_RS rs;
        rs.m_friendId = info->friendID;
        rs.m_nResult = 0;
        strcpy( rs.m_szFileMD5 , info->fileMD5.toStdString().c_str() );
        rs.m_UserId = info->userID;

        m_tcpClient->SendData( (char*)&rs, sizeof(rs) );
    }
}


void UploadWorker::slot_uploadThread(char *buf, int nlen)
{
    CKernel::GetInstance()->slot_dealUploadRs(buf , nlen );
    delete [] buf;
}
