#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;
using namespace std;

// 聊天服务器主类
class ChatServer
{
public:
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg); // 初始化聊天服务器对象
    void start();                                                                      // 启动服务

private:
    void onConnection(const TcpConnectionPtr &conn);                           // 处理用户连接和断开回调函数
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time); // 处理用户读写事件回调函数
    TcpServer _server;
    EventLoop *_loop;
};

#endif