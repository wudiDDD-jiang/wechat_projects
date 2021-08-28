#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include"wechat.h"
#include"qmytcpclient.h"
#include"Packdef.h"
#include"logindialog.h"
#include"roomdialog.h"
#include"video_read.h"
#include"audio_read.h"
#include"audio_write.h"
#include"notify.h"
#include"chatdialog.h"
#include<QMap>
#include"useritem.h"
#include<QTimer>
#include<QSystemTrayIcon>
#include"fileitem.h"
#include<QThread>

class CKernel;
typedef void (CKernel::*PFUN)(char* buf,int nlen);

class UploadWorker:public QObject
{
    Q_OBJECT
public slots:
    void slot_uploadThread( char* buf,int nlen );
};

class CKernel : public QObject
{
    Q_OBJECT
private:
    explicit CKernel(QObject *parent = 0);
    ~CKernel(){}



public slots:
    static CKernel* GetInstance();
    void DestroyInstance();
    void InitConfig();
    void setQss(QString qss);
    void setNetPackMap();
    ///////////////////
    /// 槽函数
    ///
    void slot_disConnect();
    void slot_heartConnect();
    void slot_quit();
    void activeTray(QSystemTrayIcon::ActivationReason reason);
    void slot_LoginCommit(QString name ,QString password);
    void slot_RegisterCommit(QString name ,QString password);
    void slot_addFriend();
    void slot_UserItemClicked();
    void slot_createRoom();
    void slot_joinRoom();
    void slot_leaveRoom();
    void slot_ChangeStyle(QString style);
    void slot_SendChatRq(int id,QString content);
    void slot_videoItemClicked(int id);
    void slot_sendVideoFrame(QImage& img);
    void slot_sendAudioFrame(QByteArray byte);
    void slot_refreshImage(int id, QImage &img);
    void slot_SendFileRq( int id , QString strPath);
    void slot_updateFileProcess( QString fileMD5 , qint64 cur);
    void slot_fileAccept();
    void slot_fileReject();
    ///////////
    ///  网络处理
    ///
    void slot_ReadyData(char* buf,int nlen);
    void slot_dealLoginRs(char* buf,int nlen);
    void slot_dealRegisterRs( char * buf,  int nlen);
    void slot_dealForceOffline( char * buf,  int nlen);
    void slot_dealAddFriendRq(char * buf,int nlen);
    void slot_dealAddFriendRs(char *buf, int nlen);
    void slot_dealFriendInfo(char *buf, int nlen);
    void slot_dealOfflineRs(char *buf, int nlen);
    void slot_dealChatRq(char *buf, int nlen);
    void slot_dealChatRs(char *buf, int nlen);
    void slot_dealCreateRoomRs(char *buf, int nlen);
    void slot_dealJoinRoomRs(char *buf, int nlen);
    void slot_dealRoomMember(char *buf, int nlen);
    void slot_dealLeaveRoomRs(char *buf, int nlen);
    void slot_dealVideoFrame(char *buf, int nlen);
    void slot_dealAudioFrame(char *buf, int nlen);
    void slot_dealUploadRs(char *buf, int nlen);
    void slot_dealUploadRq(char *buf, int nlen);
    void slot_dealFileBlockRq(char *buf, int nlen);
    /////////////


signals:
    void SIG_updateFileProcess( QString fileMD5 , qint64 cur);
    void SIG_dealUploadRs( char *buf, int nlen );

public:
private:
    QString m_serverIP;
    WeChat * m_weChatDlg;
    QMyTcpClient * m_tcpClient;
    PFUN m_NetPackFunMap[DEF_PACK_COUNT];
    LoginDialog *m_loginDlg;

    int m_id;
    QString m_UserName;
    QString m_feeling;
    int m_iconID;
    int m_roomid;

    RoomDialog *m_roomdlg;
    QMap<int , UserItem* > m_mapIDToUserItem;
    QMap<int , ChatDialog* > m_mapIDToChatdlg;

    QMap< int , VideoItem* > m_mapIDToVideoItem;
    QMap< int , Audio_Write* > m_mapIDToAudioWrite;
    QMap<QString , FileInfo*> m_mapFileMD5ToFileInfo;
    QMap<QString , FileItem*> m_mapFileMD5ToFileItem;

    Video_Read * m_pVideoRead;
    Audio_Read * m_pAudioRead; //声音采集

    QTimer *m_heartTimer;

    QSystemTrayIcon *m_SystemTrayIcon;

    UploadWorker* m_uploadWorker;
    QThread* m_uploadThread;
};

#endif // CKERNEL_H
