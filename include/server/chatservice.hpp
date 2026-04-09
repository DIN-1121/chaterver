#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include "UserModel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "mysqldb/db.h"
#include "public.hpp"
#include "redis.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;  // 使用nlohmann库的json命名
using namespace placeholders; // 使用占位符命名空间，方便绑定函数参数

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>; // 定义消息处理函数类型

// 聊天服务器业务类
class ChatService
{
public:
    static ChatService *instance();                                         // 获取单例对象的接口函数
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);     // 处理登录业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);       // 处理注册业务
    MsgHandler getMsgHandler(int msgid);                                    // 获取消息对应处理器
    void clientCloseException(const TcpConnectionPtr &conn);                // 处理客户端异常退出
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 一对一聊天业务
    void reset();                                                           // 重置聊天服务器，清理服务器资源和状态
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time); // 添加好友业务

    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time); // 创建群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);    // 加入群组业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 群聊业务

    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time); // 处理退出登录业务

    void handleRedisSubscribeMessage(int channel, string message); // 处理Redis订阅消息的回调函数
private:
    ChatService();                                 // 私有化构造函数，单例模式
    unordered_map<int, MsgHandler> _msgHandlerMap; // 存储消息id和对应的消息处理函数的映射关系

    UserModel _userModel; // 用户数据操作类对象

    unordered_map<int, TcpConnectionPtr> _userConnMap; // 存储在线用户的通信连接，key是用户id，value是用户的通信连接

    std::mutex _userConnMutex; // 定义互斥锁，保护_userConnMap的线程安全

    OfflineMsgModel _offlineMsgModel; // 离线消息数据操作类对象
    FriendModel _friendModel;         // 好友关系数据操作类对象
    GroupModel _groupModel;           // 群组数据操作类对象

    Redis _redis; // Redis操作对象，用于消息发布和订阅
};

#endif