#include "chatservice.hpp"
#include "public.hpp"
#include "offlinemessagemodel.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <iostream>
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service; // 定义静态局部变量，保证线程安全
    return &service;            // 返回单例对象的指针
}

// 注册消息id和对应的消息处理函数的映射关系
ChatService::ChatService()
{
    // LOGIN_MSG=1, REG_MSG=2
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});          // 将登录消息id和对应的处理函数绑定到消息处理函数映射表中
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});              // 将注册消息id和对应的处理函数绑定到消息处理函数映射表中
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});     // 将一对一聊天消息id和对应的处理函数绑定到消息处理函数映射表中
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)}); // 将添加好友消息id和对应的处理函数绑定到消息处理函数映射表中

    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)}); // 将创建群组消息id和对应的处理函数绑定到消息处理函数映射表中
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});       // 将加入群组消息id和对应的处理函数绑定到消息处理函数映射表中
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});     // 将群聊消息id和对应的处理函数绑定到

    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::clientCloseException, this, _1)}); // 将注销消息id和对应的处理函数绑定到消息处理函数映射表中

    if (_redis.connect()) // 连接Redis服务器，成功返回true，失败返回false
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2)); // 初始化Redis消息订阅处理函数，绑定到ChatService类的成员函数handleRedisSubscribeMessage
    }
}

// 获取消息对应处理器
MsgHandler ChatService::getMsgHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的处理函数
    auto it = _msgHandlerMap.find(msgid); // 在消息处理函数映射表中查找消息id对应的处理函数
    if (it == _msgHandlerMap.end())       // 如果没有找到，返回一个默认的处理函数，表示消息id没有对应的处理器
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            // cout << "msgid:" << msgid << " can not find handler!" << endl;
            LOG_ERROR << "msgid:" << msgid << " can not find handler!"; // 记录错误日志
        };
    }
    else
    {
        return it->second; // 返回找到的处理函数
        // return _msgHandlerMap[msgid]; // 返回找到的处理函数
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // cout << "do login service" << endl;
    // LOG_INFO << "do login service"; // 记录日志，表示正在处理登录业务
    int id = js["id"].get<int>();                             // 从json对象中获取用户id
    string password = js["password"].get<string>();           // 从json对象中获取密码
    User user = _userModel.query(id);                         // 根据用户id查询用户信息，返回用户对象
    if (user.getId() == id && user.getPassword() == password) // 如果用户id和密码匹配，返回登录成功的响应
    {
        if (user.getState() == "online") // 如果用户已经在线，返回重复登录的响应
        {
            json response;                                         // 创建json对象，表示重复登录的响应
            response["msgid"] = LOGIN_MSG_ACK;                     // 设置消息id，表示重复登录的响应
            response["errno"] = 2;                                 // 设置错误码，2表示重复登录
            response["errmsg"] = "该账号已经登录，请勿重复登录！"; // 设置错误信息，提示用户账号已经登录
            conn->send(response.dump());                           // 将响应转换为字符串，并发送给客户端
            // return;
        }
        else
        {
            {
                lock_guard<mutex> lock(_userConnMutex); // 加锁，保护在线用户的通信连接映射表的线程安全
                _userConnMap.insert({id, conn});        // 将用户id和对应的通信连接存储到在线用户的通信连接映射表中
            }

            _redis.subscribe(id); // 订阅用户id对应的频道，接收该用户的消息

            user.setState("online");      // 设置用户状态为在线
            _userModel.updateState(user); // 更新用户状态到数据库

            json response;                     // 创建json对象，表示登录成功的响应
            response["msgid"] = LOGIN_MSG_ACK; // 设置消息id，表示登录成功的响应
            response["errno"] = 0;             // 设置错误码，0表示登录成功
            response["id"] = user.getId();     // 设置用户id，返回给客户端
            response["name"] = user.getName(); // 设置用户名称，返回给客户端

            vector<string> offlineMsgVec = _offlineMsgModel.query(id); // 查询用户的离线消息，返回一个字符串向量
            if (!offlineMsgVec.empty())                                // 如果有离线消息，添加到响应中
            {
                response["offlinemsg"] = offlineMsgVec; // 设置离线消息，返回给客户端
                _offlineMsgModel.remove(id);            // 删除用户的离线消息
            }

            vector<User> friendVec = _friendModel.query(id); // 查询用户的好友列表，返回一个用户对象向量
            if (!friendVec.empty())                          // 如果有好友，添加到响应中
            {
                vector<string> friendVecStr;             // 定义一个字符串向量，用于存储好友列表的字符串表示
                for (const auto &friendUser : friendVec) // 遍历好友列表，将每个好友的用户信息转换为字符串，并添加到字符串向量中
                {
                    // string friendStr = to_string(friendUser.getId()) + ":" + friendUser.getName() + ":" + friendUser.getState(); // 将好友的用户id、名称和状态拼接成一个字符串，格式为"id:name:state"
                    json friendJson;                             // 定义一个json对象，用于存储好友的用户信息
                    friendJson["id"] = friendUser.getId();       // 设置好友的用户id
                    friendJson["name"] = friendUser.getName();   // 设置好友的用户名
                    friendJson["state"] = friendUser.getState(); // 设置好友的用户状态
                    friendVecStr.push_back(friendJson.dump());   // 将好友字符串添加到字符串向量中
                }
                response["friends"] = friendVecStr; // 设置好友列表，返回给客户端
            }

            // 查询用户所在的群组
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> groupVecStr;
                for (auto &group : groupVec)
                {
                    json groupJson;                      // 定义一个json对象，用于存储群组信息
                    groupJson["id"] = group.getId();     // 设置群组id
                    groupJson["name"] = group.getName(); // 设置群组名称
                    groupJson["desc"] = group.getDesc(); // 设置群组描述

                    vector<string> userVecStr; // 定义一个字符串向量，用于存储群成员的字符串表示
                    for (auto &user : group.getUsers())
                    {
                        json userJson;                         // 定义一个json对象，用于存储群成员的用户信息
                        userJson["id"] = user.getId();         // 设置群成员的用户id
                        userJson["name"] = user.getName();     // 设置群成员的用户名
                        userJson["state"] = user.getState();   // 设置群成员的用户状态
                        userJson["role"] = user.getRole();     // 设置群成员的角色
                        userVecStr.push_back(userJson.dump()); // 将群成员字符串添加到字符串向量中
                    }
                    groupJson["users"] = userVecStr;         // 设置群成员列表，返回给客户端
                    groupVecStr.push_back(groupJson.dump()); // 将群组字符串添加到字符串向量中
                }
                response["groups"] = groupVecStr; // 设置群组列表，返回给客户端
            }

            conn->send(response.dump()); // 将响应转换为字符串，并发送给客户端
        }
    }
    else // 如果用户id和密码不匹配，返回登录失败的响应
    {
        json response;                     // 创建json对象，表示登录失败的响应
        response["msgid"] = LOGIN_MSG_ACK; // 设置消息id，表示登录失败的响应
        response["errno"] = 1;             // 设置错误码，1表示登录失败
        conn->send(response.dump());       // 将响应转换为字符串，并发送给客户端
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do reg service"; // 记录日志，表示正在处理注册业务
    string name = js["name"].get<string>();         // 从json对象中获取用户名
    string password = js["password"].get<string>(); // 从json对象中获取密码
    User user;                                      // 创建用户对象
    user.setName(name);                             // 设置用户名称
    user.setPassword(password);                     // 设置用户密码
    bool state = _userModel.insert(user);           // 将用户对象插入数据库，返回插入结果
    if (state)                                      // 如果插入成功，返回注册成功的响应
    {
        json response;                   // 创建json对象，表示注册成功的响应
        response["msgid"] = REG_MSG_ACK; // 设置消息id，表示注册成功的响应
        response["errno"] = 0;           // 设置错误码，0表示注册成功
        response["id"] = user.getId();   // 设置用户id，从数据库中获取插入数据的id，并设置到响应中
        conn->send(response.dump());     // 将响应转换为字符串，并发送给客户端
    }
    else // 如果插入失败，返回注册失败的响应
    {
        json response;                   // 创建json对象，表示注册失败的响应
        response["msgid"] = REG_MSG_ACK; // 设置消息id，表示注册失败的响应
        response["errno"] = 1;           // 设置错误码，1表示注册失败
        conn->send(response.dump());     // 将响应转换为字符串，并发送给客户端
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    lock_guard<mutex> lock(_userConnMutex);                              // 加锁，保护在线用户的通信连接映射表的线程安全
    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) // 遍历在线用户的通信连接映射表，查找对应的通信连接
    {
        if (it->second == conn) // 如果找到对应的通信连接，说明该用户异常退出了
        {
            User user = _userModel.query(it->first); // 根据用户id查询用户信息，返回用户对象
            _redis.unsubscribe(it->first);           // 取消订阅用户id对应的频道，停止接收该用户的消息
            user.setState("offline");                // 设置用户状态为离线
            _userModel.updateState(user);            // 更新用户状态到数据库
            _userConnMap.erase(it);                  // 从在线用户的通信连接映射表中删除该用户的通信连接
            break;
        }
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["friendid"].get<int>(); // 从json对象中获取目标用户id
    {
        lock_guard<mutex> lock(_userConnMutex); // 加锁，保护在线用户的通信连接映射表的线程安全
        auto it = _userConnMap.find(toid);      // 在在线用户的通信连接映射表中查找目标用户的通信连接
        if (it != _userConnMap.end())           // 如果找到，说明目标用户在线，直接转发消息给目标用户
        {
            it->second->send(js.dump()); // 将消息转换为字符串，并发送给目标用户的通信连接
            return;
        }
    }

    User user = _userModel.query(toid); // 根据目标用户id查询用户信息，返回用户对象
    if (user.getState() == "online")    // 如果目标用户在线，直接转发消息给目标用户
    {
        _redis.publish(toid, js.dump()); // 将消息转换为字符串，并发布到目标用户id对应的频道，通知目标用户有新的消息
        return;
    }

    // 如果目标用户不在线，应该把消息存储到数据库中，等目标用户上线后再发送给他
    _offlineMsgModel.insert(toid, js.dump()); // 将离线消息存储到数据库中，参数是目标用户id和消息内容
}

// 重置聊天服务器，清理服务器资源和状态
void ChatService::reset()
{
    // _userModel.resetState(); // 重置用户状态，服务器异常退出后调用，确保所有用户状态为offline
    lock_guard<mutex> lock(_userConnMutex);                              // 加锁，保护在线用户的通信连接映射表的线程安全
    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) // 遍历在线用户的通信连接映射表，将所有在线用户的状态设置为离线，并更新到数据库中
    {
        User user = _userModel.query(it->first); // 根据用户id查询用户信息，返回用户对象
        _redis.unsubscribe(it->first);           // 取消订阅用户id对应的频道，停止接收该用户的消息
        user.setState("offline");                // 设置用户状态为离线
        _userModel.updateState(user);            // 更新用户状态到数据库
    }
    _userConnMap.clear(); // 清空在线用户的通信连接映射表
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();         // 从json对象中获取用户id
    int friendid = js["friendid"].get<int>(); // 从json对象中获取好友id

    _friendModel.insert(userid, friendid); // 将好友关系存储到数据库中，参数是用户id和好友id
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();                 // 从json对象中获取用户id
    string groupName = js["groupname"].get<string>(); // 从json对象中获取群组名称
    string groupDesc = js["groupdesc"].get<string>(); // 从json对象中获取群组描述

    Group group(-1, groupName, groupDesc); // 创建群组对象
    if (_groupModel.createGroup(group))    // 将群组对象插入数据库中，如果插入成功，返回创建群组成功的响应
    {
        // 将创建群组的用户添加到群组用户表中，参数是用户id、群组id和角色信息，角色信息为"creator"，表示创建者
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();                // 从json对象中获取用户id
    int groupid = js["groupid"].get<int>();          // 从json对象中获取群组id
    _groupModel.addGroup(userid, groupid, "normal"); // 将用户添加到群组用户表中，参数是用户id、群组id和角色信息，角色信息为"normal"，表示普通成员
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();                                     // 从json对象中获取用户id
    int groupid = js["groupid"].get<int>();                               // 从json对象中获取群组id
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid); // 查询群组用户列表，返回一个用户id向量，参数是用户id和群组id

    lock_guard<mutex> lock(_userConnMutex); // 加锁，保护在线用户的通信连接映射表的线程安全
    for (int id : useridVec)                // 遍历群组用户列表，获取每个用户的通信连接，并将消息转发给在线的群组成员
    {
        if (id == userid)
        {
            continue; // 跳过自己
        }
        auto it = _userConnMap.find(id); // 在在线用户的通信连接映射表中查找群组成员的通信连接
        if (it != _userConnMap.end())    // 如果找到，说明该群组成员在线，直接转发消息给该成员
        {
            it->second->send(js.dump()); // 将消息转换为字符串，并发送给该成员的通信连接
        }
        else // 如果没有找到，说明该群组成员不在线，将消息存储到数据库中作为离线消息
        {
            User user = _userModel.query(id); // 根据群组成员的用户id查询用户信息，返回用户对象
            if (user.getState() == "online")  // 如果群组成员在线，直接转发消息给该成员
            {
                _redis.publish(id, js.dump()); // 将消息转换为字符串，并发布到群组成员的用户id对应的频道，通知该成员有新的消息
            }
            else // 如果群组成员不在线，将消息存储到数据库中作为离线消息
            {
                _offlineMsgModel.insert(id, js.dump()); // 将离线消息存储到数据库中，参数是群组成员的用户id和消息内容
            }
        }
    }
}

// 处理退出登录业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 从json对象中获取用户id
    {
        lock_guard<mutex> lock(_userConnMutex); // 加锁，保护在线用户的通信连接映射表的线程安全
        auto it = _userConnMap.find(userid);    // 在在线用户的通信连接映射表中查找用户的通信连接
        if (it != _userConnMap.end())           // 如果找到，说明该用户在线，将其状态设置为离线，并从在线用户的通信连接映射表中删除该用户的通信连接
        {
            User user = _userModel.query(userid); // 根据用户id查询用户信息，返回用户对象
            _redis.unsubscribe(userid);           // 取消订阅用户id对应的频道，停止接收该用户的消息
            user.setState("offline");             // 设置用户状态为离线
            _userModel.updateState(user);         // 更新用户状态到数据库
            _userConnMap.erase(it);               // 从在线用户的通信连接映射表中删除该用户的通信连接
        }
    }
}

// 处理Redis订阅消息的回调函数
void ChatService::handleRedisSubscribeMessage(int channel, string message)
{
    lock_guard<mutex> lock(_userConnMutex); // 加锁，保护在线用户的通信连接映射表的线程安全
    auto it = _userConnMap.find(channel);   // 在在线用户的通信连接映射表中查找频道对应的通信连接，频道对应的通信连接就是用户的通信连接
    if (it != _userConnMap.end())           // 如果找到，说明用户在线，直接转发消息给用户
    {
        it->second->send(message); // 将消息转换为字符串，并发送给用户的通信连接
        return;
    }

    _offlineMsgModel.insert(channel, message); // 将离线消息存储到数据库中，参数是用户id和消息内容
}