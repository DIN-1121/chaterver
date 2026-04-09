#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json; // 使用nlohmann库的json命名空间

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册用户断开与连接回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); //_1占位符，表示第一个参数
    // 注册用户读写事件回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3)); //_2和_3占位符，表示第二个和第三个参数
    //_server.setWriteCompleteCallback(std::bind(&ChatServer::onWriteComplete, this, _1));
    // 设置线程数量，muduo库会自动分配I/O线程和工作线程  一个io线程，三个工作线程
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start(); // 开启事件循环，监听事件，等待用户连接和请求
}

void ChatServer::onConnection(const TcpConnectionPtr &conn) // 处理用户连接和断开回调函数
{
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn); // 处理客户端异常退出，更新用户状态，删除在线用户的通信连接
        conn->shutdown();                                    // 关闭连接
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) // 处理用户读写事件回调函数
{
    string recvMsg = buf->retrieveAllAsString(); // 从缓冲区中读取数据，并转换为字符串
    json js = json::parse(recvMsg);              // 将字符串解析为json对象
    // 完全解耦网络模块代码和业务模块代码，网络模块只负责数据的传输，业务模块负责数据的处理
    ChatService::instance()->getMsgHandler(js["msgid"].get<int>())(conn, js, time); // 获取消息id对应的处理器，并调用处理器函数，传入连接对象、json对象和时间戳
}