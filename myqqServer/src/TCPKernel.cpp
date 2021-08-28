#include<TCPKernel.h>
#include "packdef.h"
#include<stdio.h>
//#include<time.h>
#include<sys/time.h>

using namespace std;


////注册
//#define  DEF_PACK_REGISTER_RQ    (DEF_PACK_BASE + 0)
//#define  DEF_PACK_REGISTER_RS    (DEF_PACK_BASE + 1)
////登录
//#define  DEF_PACK_LOGIN_RQ    (DEF_PACK_BASE + 2)
//#define  DEF_PACK_LOGIN_RS    (DEF_PACK_BASE + 3)

static const ProtocolMap m_ProtocolMapEntries[] =
{
    {DEF_PACK_REGISTER_RQ , &TcpKernel::RegisterRq},
    {DEF_PACK_LOGIN_RQ , &TcpKernel::LoginRq},
    {DEF_PACK_ADD_FRIEND_RQ , &TcpKernel::AddFriendRq},
    {DEF_PACK_ADD_FRIEND_RS , &TcpKernel::AddFriendRs},
    { DEF_PACK_OFFLINRE_RQ , &TcpKernel::OfflineRq},
    { DEF_PACK_CHAT_RQ , &TcpKernel::ChatRq},
    {DEF_PACK_CREATEROOM_RQ , &TcpKernel::CreateRoomRq},
    {DEF_PACK_JOINROOM_RQ , &TcpKernel::JoinRoomRq},
    {DEF_PACK_LEAVEROOM_RQ , &TcpKernel::LeaveRoomRq},
    {DEF_PACK_VIDEO_FRAME , &TcpKernel::VideoFrame},
    {DEF_PACK_AUDIO_FRAME , &TcpKernel::AudioFrame},
    {DEF_PACK_UPLOAD_RQ , &TcpKernel::UploadRq},
    {DEF_PACK_UPLOAD_RS , &TcpKernel::UploadRs},
    {DEF_PACK_FILEBLOCK_RQ , &TcpKernel::FileBlockRq},
    {DEF_PACK_HEART , &TcpKernel::HeartRq},
    {0,0}
};
#define RootPath   "/home/qiqi/tmp/Video/"


TcpKernel* TcpKernel::m_pStaticThis = NULL;

int TcpKernel::Open()
{
    m_pStaticThis = this;
    initRand();
    m_sql = new CMysql;
    m_tcp = new TcpNet(this);
    m_tcp->SetpThis(m_tcp);
    pthread_mutex_init(&m_tcp->alock,NULL);
    pthread_mutex_init(&m_tcp->rlock,NULL);
    if(  !m_sql->ConnectMysql("localhost","root","jiang","wechat")  )
    {
        printf("Conncet Mysql Failed...\n");
        return FALSE;
    }
    else
    {
        printf("MySql Connect Success...\n");
    }
    if( !m_tcp->InitNetWork()  )
    {
        printf("InitNetWork Failed...\n");
        return FALSE;
    }
    else
    {
        printf("Init Net Success...\n");
    }
    //创建检查心跳线程
//    m_quitFlag = false;
//    int err= 0 ;
//    if((err = pthread_create(&m_heartThread,NULL, CheckHeart,(void*)this)) > 0)
//    {
//        printf("create heart thread error:%s\n",strerror(err));
//        return FALSE;
//    }

    return TRUE;
}

void TcpKernel::Close()
{
    m_sql->DisConnect();
    m_tcp->UnInitNetWork();

    m_quitFlag = true;
    for( auto ite = m_mapIDToUserInfo.begin(); ite != m_mapIDToUserInfo.end(); ++ite)
    {
        delete ite->second;
    }
    m_mapIDToUserInfo.clear();
}


void TcpKernel::DealData(int clientfd,char *szbuf,int nlen)
{
    PackType *pType = (PackType*)szbuf;
    int i = 0;
    while(1)
    {
        if(*pType == m_ProtocolMapEntries[i].m_type)
        {
            PFUN fun= m_ProtocolMapEntries[i].m_pfun;
            (this->*fun)(clientfd,szbuf,nlen);
        }
        else if(m_ProtocolMapEntries[i].m_type == 0 &&
                m_ProtocolMapEntries[i].m_pfun == 0)
            return;
        ++i;
    }
    return;
}

//注册请求结果
#define userid_is_exist      0
#define register_sucess      1


//注册
void TcpKernel::RegisterRq(int clientfd,char* szbuf,int nlen)
{
    printf("clientfd:%d RegisterRq\n", clientfd);

    STRU_REGISTER_RQ * rq = (STRU_REGISTER_RQ *)szbuf;
    STRU_REGISTER_RS rs;

    //根据用户名 查用户
    char sqlStr[1024]={0}/*""*/;
    sprintf( sqlStr , "select name from t_user where name = '%s';",rq->m_szUser );
    list<string> resList;
    if( !m_sql->SelectMysql(sqlStr , 1, resList ))
    {
        printf("SelectMysql error: %s \n", sqlStr);
        return;
    }
    if( resList.size() == 0 )
    {
    //没有用户 可以注册 -- 写表
        rs.m_lResult = register_sucess;

        sprintf( sqlStr , "insert into t_user( name , password) values('%s' , '%s');"
                 ,rq->m_szUser,rq->m_szPassword );
        if( !m_sql->UpdataMysql(sqlStr ))
        {
            printf("UpdataMysql error: %s \n", sqlStr);
        }
        //t_userInfo( id  name  icon feeling)
        sprintf( sqlStr , "select id from  t_user where  name = '%s';"
                 ,rq->m_szUser);
        list<string> resID;
        m_sql->SelectMysql( sqlStr , 1 , resID);
        int id = 0;
        if( resID.size() >0  )  id = atoi( resID.front().c_str() );

        sprintf( sqlStr , "insert into t_userInfo( id , name , icon , feeling) values ( %d ,'%s' ,0 ,'');"
                 ,id,rq->m_szUser);
        m_sql->UpdataMysql( sqlStr);

    }else
    {
    //有用户 就注册失败
        rs.m_lResult = userid_is_exist;
    }
    m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs) );
}
//登录请求结果
#define userid_no_exist      0
#define password_error       1
#define login_sucess         2
#define user_online          3
//登录
void TcpKernel::LoginRq(int clientfd ,char* szbuf,int nlen)
{
    printf("clientfd:%d LoginRq\n", clientfd);

    STRU_LOGIN_RQ * rq = (STRU_LOGIN_RQ *)szbuf;
    STRU_LOGIN_RS rs;

    //查用户名 密码
    char sqlStr[1024] = "";
    sprintf( sqlStr , "select password , id from t_user where name = '%s';", rq->m_szUser );
    list<string> resList;

    if( !m_sql->SelectMysql(sqlStr , 2 , resList))
    {
        printf("SelectMysql error: %s \n", sqlStr);
        return;
    }
    if( resList .size() > 0 )
    {
    //有用户
        if( strcmp( resList.front().c_str() , rq->m_szPassword ) == 0 )
        { //密码一致
            rs.m_lResult = login_sucess;
            resList.pop_front();
            int id = atoi ( resList.front().c_str() );
            rs.m_UserID = id;

            m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs) );
 //           m_mapIDToFD[id] = clientfd;
//            if( m_mapIDToFD.find(id) != m_mapIDToFD.end() )
//            {//找到了
//                //强制前一个 下线
//                STRU_FORCE_OFFLINE offline;
//                offline.m_UserID = id;
//                m_tcp->SendData( m_mapIDToFD[id] , (char*)&offline , sizeof(offline));
//            }

//            m_mapIDToFD[id] = clientfd;

                UserInfo * info  = NULL;
                if( m_mapIDToUserInfo.find(id) != m_mapIDToUserInfo.end() )
                {//找到了
                    //强制前一个 下线
                    //如果在房间先退出来
                    info = m_mapIDToUserInfo[id];
                    if( info->m_fd != clientfd )
                    {
                        STRU_FORCE_OFFLINE offline;
                        offline.m_UserID = id;

                        if( info->m_roomid != 0 )
                        {
                            LeaveRoom( info->m_id , info->m_roomid );
                        }
                        m_tcp->SendData( info->m_fd , (char*)&offline , sizeof(offline));
                    }

                }else
                {
                    info  = new UserInfo;
                }

 //               m_mapIDToUserInfo[id] = clientfd;

                info->m_fd = clientfd;
                info->m_id = id;
                strcpy( info->m_userName , rq->m_szUser );
                info->m_state = 1;

                //去数据库同步 info
                m_mapIDToUserInfo[ info->m_id ] = info;

                // 获取好友列表 todo  t_userInfo( id  name  icon feeling)
                // 先从服务器获取用户的信息 同步到info
                GetUserInfoFromSql( info->m_id );
                //获取好友列表
                UserGetFriendList( info->m_id );

            return;
        }else
        {//密码不一致
            rs.m_lResult = password_error;
        }
    }else{
    //没有用户
        rs.m_lResult = userid_no_exist;
    }
    m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs) );
}
//先从服务器获取用户的信息 同步到info
void TcpKernel::GetUserInfoFromSql( int id )
{//首先要在线
    if( m_mapIDToUserInfo.find(id ) == m_mapIDToUserInfo.end() ) return;

    UserInfo* info = m_mapIDToUserInfo[id];
    info->m_state = 1;

    char strsql[1024] = "";
    sprintf(strsql , "select icon , feeling from t_userInfo where  id = %d;", id);
    list<string> res;
    m_sql->SelectMysql( strsql , 2 , res );
    if( res .size() == 0 ) return ;
    info->m_iconID = atoi( res.front().c_str() );
    res.pop_front();
    strcpy ( info->m_feeling  ,res.front().c_str());
    res.pop_front();

//    info->m_iconID
//    info->m_feeling
}

//用户获取好友列表(获取列表), 登录人的信息要发给在线的好友 (上线提示)
void TcpKernel::UserGetFriendList( int id )
{
    //首先保证在线
    if( m_mapIDToUserInfo.find(id ) == m_mapIDToUserInfo.end() ) return;

    UserInfo* loginer = m_mapIDToUserInfo[id];
    STRU_FRIEND_INFO loginrq;
    strcpy( loginrq.m_feeling , loginer->m_feeling );
    loginrq.m_iconID = loginer->m_iconID;
    loginrq.m_state = 1;
    strcpy( loginrq.m_szName , loginer->m_userName );
    loginrq.m_userID = loginer->m_id ;

    // 给自己也发一个状态
    m_tcp->SendData( loginer->m_fd , (char*) &loginrq , sizeof(loginrq) );

    //查表 查好友
    char sqlstr[1024]="";
    sprintf( sqlstr , "select idA from t_friend where idB = %d" , id );
    list<string> resID;
    m_sql->SelectMysql( sqlstr , 1 , resID );
    if( resID.size() == 0 ) return;
    int ncount = resID.size();
    for( int i = 0 ; i < ncount ; ++i)//遍历好友
    {
        int friendid = atoi( resID.front().c_str() );
        resID.pop_front();

        STRU_FRIEND_INFO friendrq; //写好友信息
        if( m_mapIDToUserInfo.find( friendid ) != m_mapIDToUserInfo.end() )
        {//好友在线  取info 然后 登录人的信息发送给好友
            UserInfo * friender = m_mapIDToUserInfo[friendid];
            strcpy( friendrq.m_feeling , friender->m_feeling );
            friendrq.m_iconID = friender->m_iconID;
            friendrq.m_state = 1;
            strcpy( friendrq.m_szName , friender->m_userName );
            friendrq.m_userID = friender->m_id ;

            m_tcp->SendData( friender->m_fd , (char*)&loginrq , sizeof(loginrq) );
        }else
        {//好友不再线  从数据库取
            friendrq.m_state = 0;
            friendrq.m_userID = friendid;

            char strsql[1024] = "";
            sprintf(strsql , "select name, icon , feeling from t_userInfo where id = %d;", friendid);
            list<string> res;
            m_sql->SelectMysql( strsql , 3 , res );
            if( res .size() == 0 ) return ;

            strcpy ( friendrq.m_szName , res.front().c_str());
            res.pop_front();
            friendrq.m_iconID = atoi( res.front().c_str() );
            res.pop_front();
            strcpy ( friendrq.m_feeling  ,res.front().c_str());
            res.pop_front();
        }
        // 好友的信息发送给登录者
        m_tcp->SendData( loginer->m_fd , (char*) &friendrq , sizeof(friendrq) );
    }

}

//创建房间请求
void TcpKernel::CreateRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d CreateRoomRq\n", clientfd);

    //拆包
    STRU_CREATEROOM_RQ *rq = (STRU_CREATEROOM_RQ *)szbuf;
     if( m_mapIDToUserInfo.find( rq->m_UserID ) == m_mapIDToUserInfo.end() ) return;
    // map 查看有没有 user
    UserInfo* info = m_mapIDToUserInfo[ rq->m_UserID];
    // 随机roomid 判定是否重复
     int roomid = 0;
    do
     {
         roomid = rand()%999999 + 1;
     }while( roomid == 0 || m_mapRoomIDToUserList.find(roomid) != m_mapRoomIDToUserList.end() );
    // list  添加 用户 加入到map中

    info->m_roomid = roomid;
    list<UserInfo*> lst;
    lst.push_back(info);
    m_mapRoomIDToUserList[ roomid ] = lst;
    // 回复信息 rs
    STRU_CREATEROOM_RS rs;
    rs.m_RoomId = roomid;
    rs.m_lResult = 1;

    m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs));
}

//加入房间请求处理
void TcpKernel::JoinRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d JoinRoomRq\n", clientfd);

    //拆包
    STRU_JOINROOM_RQ *rq = (STRU_JOINROOM_RQ *)szbuf;
    STRU_JOINROOM_RS rs;
    //先看map中有没有 这个人  没有返回
     if( m_mapIDToUserInfo.find( rq->m_UserID ) == m_mapIDToUserInfo.end() )
     {
         rs.m_RoomID = 0;
         rs.m_lResult = 0;
         m_tcp->SendData(  clientfd , (char*)&rs , sizeof(rs) );
         return;
     }
    UserInfo * joiner =  m_mapIDToUserInfo[rq->m_UserID];

    //再看房间有没有 有 没有 返回
    if( m_mapRoomIDToUserList.find( rq->m_RoomID ) == m_mapRoomIDToUserList.end() )
    {
        rs.m_RoomID = rq->m_RoomID;
        rs.m_lResult = 0;
        m_tcp->SendData(  clientfd , (char*)&rs , sizeof(rs) );
        return;
    }
    list<UserInfo* >  lst = m_mapRoomIDToUserList[ rq->m_RoomID ];  //假定 map list 节点都是有效的.

    rs.m_RoomID = rq->m_RoomID;
    rs.m_lResult = 1;
    m_tcp->SendData(  clientfd , (char*)&rs , sizeof(rs) );

    joiner->m_roomid = rq->m_RoomID;
    //有这个房间  写  加入人的房间成员信息  获取房间成员的列表
    STRU_ROOM_MEMBER_RQ joinrq;
    joinrq.m_UserID = rq->m_UserID;
    strcpy( joinrq.m_szUser  , joiner->m_userName );

    //遍历列表 -- 写房间内人的成员信息
    for( auto ite = lst.begin() ; ite!= lst.end() ; ++ite)
    {
        UserInfo*  inner = *ite;
        STRU_ROOM_MEMBER_RQ innerrq;
        innerrq.m_UserID = inner->m_id;
        strcpy( innerrq.m_szUser  , inner->m_userName );

        // 加入人的信息发送给房间内所有人
        m_tcp->SendData( inner->m_fd , (char*)&joinrq , sizeof(joinrq) );
        // 房间内所有人信息 发送给加入人
        m_tcp->SendData( joiner->m_fd , (char*)&innerrq , sizeof(innerrq) );
    }
    // 加入人的信息添加到list , 然后更新map
    lst.push_back( joiner );
    m_mapRoomIDToUserList[ rq->m_RoomID ] = lst;

}

//处理离开房间请求
void TcpKernel::LeaveRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d LeaveRoomRq\n", clientfd);

    //拆包
    STRU_LEAVEROOM_RQ *rq = (STRU_LEAVEROOM_RQ *)szbuf;

    LeaveRoom(rq->m_nUserId ,rq->m_RoomId );
}

// 身份为id的人退出房间
void TcpKernel::LeaveRoom(int id , int roomid)
{
    //看 map 有没有这个人 没有 不处理
     if( m_mapIDToUserInfo.find( id ) == m_mapIDToUserInfo.end() ) return;
     UserInfo * leaver = m_mapIDToUserInfo[ id ];

    // 看 map 看房间有没有 没有 不处理
    if( m_mapRoomIDToUserList.find( roomid ) == m_mapRoomIDToUserList.end() ) return;
    list<UserInfo*> lst = m_mapRoomIDToUserList[ roomid ];
    //map中找到房间  list

    STRU_LEAVEROOM_RS rs;
    rs.m_UserID = leaver->m_id;
    strcpy( rs.szUserName , leaver->m_userName );
    //遍历list  不是这个人 发这个人离开   是这个人 , erase该节点
    for( auto ite = lst.begin() ; ite != lst.end(); )
    {
        UserInfo * inner = *ite ;

        if( inner ->m_id  != leaver->m_id  )
        {
            m_tcp->SendData( inner->m_fd , (char*)&rs , sizeof(rs ));
            ++ite;
        }else
        {
            ite = lst.erase(ite);
        }
    }
    //更新map

    //如果离开房间后,成员数量为0 , 那么从map中去除.
    if( lst.size() == 0 )
    {
        m_mapRoomIDToUserList.erase( roomid );
    }else
    {
        m_mapRoomIDToUserList[roomid] = lst;
    }
}

//处理视频帧
void TcpKernel::VideoFrame(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d VideoFrame\n", clientfd);
    ///视频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray videoFrame;

    //拆包
    char *tmp = szbuf;
    int type = *(int*) tmp ;
    tmp += sizeof( int );
    int userId = *(int*) tmp;
    tmp += sizeof( int );
    int roomId = *(int*) tmp;
    tmp += sizeof( int );
    //找到map里面房间 , 转发给其他人
    if( m_mapRoomIDToUserList.find( roomId ) == m_mapRoomIDToUserList.end() ) return;
    list<UserInfo*> lst = m_mapRoomIDToUserList[roomId];
    for( auto ite = lst.begin() ; ite!=lst.end() ; ++ite)
    {
        UserInfo* user = *ite;
        if( user->m_id != userId )
        {
            SendMsgToId( user->m_id , szbuf , nlen );
        }
    }
}

//处理音频帧
void TcpKernel::AudioFrame(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d AudioFrame\n", clientfd);
    ///音频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// QByteArray audioFrame;

    //拆包
    char *tmp = szbuf;
    int type = *(int*) tmp ;
    tmp += sizeof( int );
    int userId = *(int*) tmp;
    tmp += sizeof( int );
    int roomId = *(int*) tmp;
    tmp += sizeof( int );
    //找到map里面房间 , 转发给其他人
    if( m_mapRoomIDToUserList.find( roomId ) == m_mapRoomIDToUserList.end() ) return;
    list<UserInfo*> lst = m_mapRoomIDToUserList[roomId];
    for( auto ite = lst.begin() ; ite!=lst.end() ; ++ite)
    {
        UserInfo* user = *ite;
        if( user->m_id != userId )
        {
            SendMsgToId( user->m_id , szbuf , nlen );
        }
    }
}

#include <sys/stat.h>
#include <sys/types.h>

//上传请求
void TcpKernel::UploadRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d UploadRq\n", clientfd);

    STRU_UPLOAD_RQ *rq = (STRU_UPLOAD_RQ *)szbuf;

    if( m_mapIDToUserInfo.find( rq->m_friendId) != m_mapIDToUserInfo.end() )
    {
        this->SendMsgToId( rq->m_friendId , szbuf , nlen );

        FileInfo * info = new FileInfo;
        strcpy( info->fileMD5 , rq->m_szFileMD5 );
        strcpy( info->fileName , rq->m_szFileName );

        //创建文件夹
        char pathToID[64];
        sprintf(pathToID, "/home/qiqi/tmp/%d", rq->m_UserId);
        mkdir(pathToID,S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);

        sprintf( info->filePath , "/home/qiqi/tmp/%d/%s", rq->m_UserId,rq->m_szFileName );
        info->filePos = 0;
        info->fileSize = rq->m_nFileSize;
        info->pFile = NULL;

        info->userID = rq->m_UserId;
        info->friendID = rq->m_friendId;

        string strMD5 = info->fileMD5;
        m_mapFileMD5ToFileInfo[ strMD5 ] = info;

    }else
    {
        STRU_UPLOAD_RS rs ;
        rs.m_friendId = rq->m_friendId;
        rs.m_nResult = 0;
        strcpy( rs.m_szFileMD5 , rq->m_szFileMD5 );
        rs.m_UserId = rq->m_UserId;
        m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs));
    }



}

void TcpKernel::UploadRs(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d UploadRs\n", clientfd);
    STRU_UPLOAD_RS * rs = (STRU_UPLOAD_RS* )szbuf;

    string strMD5 =  rs->m_szFileMD5;
    if( m_mapFileMD5ToFileInfo .find( strMD5 ) != m_mapFileMD5ToFileInfo.end() )
    {
        if( rs->m_nResult == 0 )
        {
             m_mapFileMD5ToFileInfo.erase( strMD5 );
        }else
        {
            FileInfo* info = m_mapFileMD5ToFileInfo[ strMD5 ];
            info->pFile = fopen( info->filePath ,"w");
        }
    }

    SendMsgToId( rs->m_UserId , szbuf, nlen );
}

//上传文件块请求
void TcpKernel::FileBlockRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d FileBlockRq\n", clientfd);
    //拆包
    STRU_FILEBLOCK_RQ * rq = (STRU_FILEBLOCK_RQ *)szbuf;
    string strMD5 = rq->m_szFileMD5;
    if( m_mapFileMD5ToFileInfo.find( strMD5 ) != m_mapFileMD5ToFileInfo.end() )
    {

        FileInfo * pInfo = m_mapFileMD5ToFileInfo[strMD5] ;

        int res = fwrite( rq->m_szFileContent , 1 , rq->m_nBlockLen , pInfo->pFile );

        pInfo->filePos += res;
        if( pInfo->filePos >= pInfo->fileSize )
        {
            fclose( pInfo->pFile );
            pInfo->pFile = NULL;
            // 开始向 c2 发送文件  开启一个线程发送
            pthread_t pid ;
            int err = 0;
            if((err = pthread_create(&pid,NULL, FileBlockSendThread ,(void*)pInfo)) > 0)
            {
                printf("if find\n");
                printf("create FileBlockSendThread error:%s\n",strerror(err));
                return ;
            }
        }
    }
}

//处理心跳
void TcpKernel::HeartRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d HeartRq\n", clientfd);

    STRU_HEART * rq = (STRU_HEART *)szbuf;
    if( m_mapIDToUserInfo.find( rq->m_nUserId ) != m_mapIDToUserInfo.end() )
    {
        UserInfo* info = m_mapIDToUserInfo[rq->m_nUserId ];

        info->m_heartCount = 300;
    }
}

//检查心跳的线程函数
void * TcpKernel::CheckHeart(void *arg)
{
    TcpKernel* pthis = (TcpKernel*)arg;

    int nCount = 0;
    while( /*!pthis->m_quitFlag*/1 )
    {
        //3s以上检测一次
        if( nCount >= 3 )
        {
            //printf("check heart\n");
            nCount = 0;
            for( auto ite = pthis->m_mapIDToUserInfo.begin();
                 ite != pthis->m_mapIDToUserInfo.end();++ite )
            {
                UserInfo * info = ite->second;
                info->m_heartCount -= 100;
                if( info->m_heartCount < 0 )
                {
                    pthis->Offline( info->m_id );
                }
            }
        }
        sleep( 1 );
        nCount ++;
    }
    return 0;
}

//发送文件流程线程
void *TcpKernel::FileBlockSendThread(void *arg)
{
    printf("FileBlockSendThread\n");

    FileInfo * pInfo = (FileInfo *)arg;


    pInfo->pFile = fopen(pInfo->filePath , "r");
    pInfo->filePos = 0;

    if( pInfo->pFile )
    {
        while(1)
        {
            STRU_FILEBLOCK_RQ rq;
            rq.m_friendId = pInfo->friendID;
            rq.m_nUserId = pInfo->userID;
            strcpy( rq.m_szFileMD5 , pInfo->fileMD5 );
            rq.m_nBlockLen = fread ( rq.m_szFileContent , 1, MAX_CONTENT_LEN, pInfo->pFile);

            pInfo->filePos += rq.m_nBlockLen;

            TcpKernel::m_pStaticThis->SendMsgToId( pInfo->friendID , (char*)&rq , sizeof(rq) );

            if( pInfo->filePos >= pInfo->fileSize )
            {
                break;
            }
        }
        fclose(pInfo->pFile);
    }
    delete pInfo;
}

//随机数初始化
void TcpKernel::initRand()
{
    struct timeval time;
    gettimeofday( &time , NULL);
    srand( time.tv_sec + time.tv_usec );
}

//处理添加好友请求
void TcpKernel::AddFriendRq(int clientfd ,char* szbuf,int nlen)
{
    printf("clientfd:%d AddFriendRq\n", clientfd);

    STRU_ADD_FRIEND_RQ *rq = (STRU_ADD_FRIEND_RQ *)szbuf;
    STRU_ADD_FRIEND_RS rs;
    //根据用户名 拿到 id
    char strSql[1024] ="";
    sprintf(strSql , "select id from t_user where name = '%s';",rq->m_szAddFriendName);
    //rq->m_szAddFriendName
    list<string> resID;
    if( !m_sql->SelectMysql(strSql , 1 , resID))
    {
        printf(" SelectMysql Error : %s\n", strSql);
        return;
    }//是否有这个用户
    if( resID.size() == 0 )
    {
        rs.m_result = no_this_user;
    }else
    {//看是否在线
        int id = atoi( resID.front().c_str() );
        if( m_mapIDToUserInfo.find(id) != m_mapIDToUserInfo.end() )
        {//在线 转发
            SendMsgToId( id , szbuf , nlen);
            return;
        }else
        {//不在线 返回 不在线
            rs.m_result = user_is_offline;
        }
    }
    m_tcp->SendData( clientfd, (char*)&rs , sizeof(rs));
}

//处理添加好友回复
void TcpKernel::AddFriendRs(int clientfd ,char* szbuf,int nlen)
{
    printf("clientfd:%d AddFriendRs\n", clientfd);

    STRU_ADD_FRIEND_RS * rs = (STRU_ADD_FRIEND_RS *)szbuf;

    //rs->m_result
    if( rs->m_result == add_success )
    {//同意 写数据库   id A->B  B->A  更新好友列表

        char strSql[1024] = "";
        sprintf( strSql , "insert into t_friend (idA,idB) values(%d , %d);"
                 , rs->m_userID , rs->m_friendID );

        m_sql->UpdataMysql( strSql );

        sprintf( strSql , "insert into t_friend (idA,idB) values(%d , %d);"
                  , rs->m_friendID, rs->m_userID);

        m_sql->UpdataMysql( strSql );

        //更新好友列表 todo
        UserGetFriendList( rs->m_userID );

    }

    SendMsgToId( rs->m_userID , szbuf , nlen );

    //看结果 拒绝  同意
    //拒绝

    //同意 写数据库   id A->B  B->A  更新好友列表
    //转发
}

//处理离线
void TcpKernel::OfflineRq(int clientfd ,char* szbuf,int nlen)
{//拆包
    printf("clientfd:%d OfflineRq\n", clientfd);
    STRU_OFFLINE_RQ * rq = (STRU_OFFLINE_RQ *)szbuf;

    Offline(rq->m_userID);
}

//身份为id的人 离线
void TcpKernel::Offline(int id)
{
    char sqlbuf[ _DEF_SQLIEN ] = "";
    sprintf(sqlbuf, "select idB from t_friend where idA = '%d';", id);
    list<string> lst;
    m_sql->SelectMysql( sqlbuf, 1,lst);

    //解除映射关系
    auto ite = m_mapIDToUserInfo.find(id);
    if(  ite != m_mapIDToUserInfo.end() )
    {
        UserInfo* user = ite->second;

        //在房间里推出房间, 并给房间内成员发离开

        if(user->m_roomid != 0 )
        {
            // user这个人 离开房间 告诉房间内人
            LeaveRoom( user->m_id , user->m_roomid );
        }

        m_mapIDToUserInfo.erase( id );
        delete user;
    }

    //给所有好友发离线
    while( lst.size() > 0 )
    {
        int friendid = atoi( lst.front().c_str() );
        lst.pop_front();
        STRU_OFFLINE_RS rs;
        rs.m_userID = id;

        SendMsgToId(  friendid , (char*)&rs , sizeof(rs) );
    }
}



//聊天请求处理
void TcpKernel::ChatRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d ChatRq\n", clientfd);

    STRU_CHAT_RQ* rq = (STRU_CHAT_RQ*)szbuf;

    if( m_mapIDToUserInfo.find( rq->m_friendID ) != m_mapIDToUserInfo.end() )
    {
        SendMsgToId( rq->m_friendID , szbuf , nlen);
    }else
    {
        STRU_CHAT_RS rs;
        rs.m_result = user_is_offline;
        rs.m_userID = rq->m_userID;
        rs.m_friendID = rq->m_friendID;

        m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs ) );
    }
}

//根据id转发
void TcpKernel::SendMsgToId( int id  , char* szbuf ,int nlen)
{
    //用户在线转发
    if( m_mapIDToUserInfo.find( id ) != m_mapIDToUserInfo.end() )
    {
        UserInfo * user = m_mapIDToUserInfo[id];
        m_tcp->SendData( user->m_fd ,szbuf , nlen );
    }
}

