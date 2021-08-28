#ifndef _TCPKERNEL_H
#define _TCPKERNEL_H



#include "TCPNet.h"
#include "Mysql.h"

class TcpKernel;
typedef void (TcpKernel::*PFUN)(int,char*,int nlen);

typedef struct
{
    PackType m_type;
    PFUN m_pfun;
} ProtocolMap;



class TcpKernel:public IKernel
{
public:
    int Open();

    void Close();

    void DealData(int clientfd, char*szbuf, int nlen);

    //注册
    void RegisterRq(int clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(int clientfd, char*szbuf, int nlen);

    void AddFriendRq(int clientfd, char*szbuf, int nlen);

    void AddFriendRs(int clientfd, char*szbuf, int nlen);

    void OfflineRq(int clientfd, char*szbuf, int nlen);

    void Offline(int id);

    void ChatRq(int clientfd, char*szbuf, int nlen);

    void SendMsgToId(int id, char *, int nlen);

    void GetUserInfoFromSql(int id);

    void UserGetFriendList(int id);

    void CreateRoomRq(int clientfd, char*szbuf, int nlen);

    void JoinRoomRq(int clientfd, char*szbuf, int nlen);

    void LeaveRoomRq(int clientfd, char*szbuf, int nlen);

    void LeaveRoom(int id, int roomid);

    void VideoFrame(int clientfd, char*szbuf, int nlen);

    void initRand();

    void AudioFrame(int clientfd, char *szbuf, int nlen);

    void UploadRq(int clientfd, char *szbuf, int nlen);

    void UploadRs(int clientfd, char *szbuf, int nlen);

    void FileBlockRq(int clientfd, char *szbuf, int nlen);


    void HeartRq(int clientfd, char *szbuf, int nlen);

    static void * CheckHeart(void * arg);

    static void *FileBlockSendThread(void * arg);
private:
    CMysql * m_sql;

    TcpNet * m_tcp;
//    map<int , int > m_mapIDToFD;
    map<int , UserInfo*> m_mapIDToUserInfo;

    map<int , list<UserInfo*> > m_mapRoomIDToUserList;

    map<string , FileInfo*> m_mapFileMD5ToFileInfo;

    pthread_t  m_heartThread;

    bool m_quitFlag;

    static TcpKernel* m_pStaticThis;
};






////读取上传的视频流信息
//void UpLoadVideoInfoRq(int,char*);
//void UpLoadVideoStreamRq(int,char*);
//void GetVideoRq(int,char*);
//char* GetVideoPath(char*);
//void QuitRq(int,char*);
//void PraiseVideoRq(int,char*);
//void GetAuthorInfoRq(int,char*);
#endif
