/*
muduo网络库给用户提供两个主要类
 TcpServer：用于创建服务器，监听客户端连接，并处理客户端请求。
 TcpClient：用于创建客户端，连接服务器，并发送请求。
除此之外，还有一个EventLoop类，负责事件循环，监听和分发事件。
 TcpServer和TcpClient，当然还有一个EventLoop类，用户在使用muduo网络库时，基本上只需要和这三个类打交道就可以了。
 TcpServer和TcpClient分别代表了服务器和客户端的功能，EventLoop则是事件循环，负责监听和分发事件。通过这三个类，用户可以轻松地构建高性能的网络应用程序。

 epoll+线程池：
 muduo网络库使用epoll作为底层的I/O多路复用机制，能够高效地处理大量的并发连接。同时，muduo还使用线程池来处理客户端请求，避免了每个请求都创建一个线程的开销，提高了性能和资源利用率。
 能够把网络 io代码块和业务逻辑分开，网络 io 由一个线程池来处理，业务逻辑由另一个线程池来处理，这样就能够充分利用多核 CPU 的性能，提高系统的吞吐量和响应速度。
*/

#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include <iostream>
#include <string>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

// 基于网络muduo库开发服务器程序
/*
1.组合TcpServer对象，创建服务器对象
2.创建EventLoop事件循环对象指针，监听事件
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在构造函数中，注册处理用户连接的回调函数onConnection，处理用户读写事件的回调函数onMessage.//处理用户写事件的回调函数onWriteComplete
5.设置合适的线程数量，muduo库会自动分配I/O线程和工作线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环对象指针
               const InetAddress &listenAddr, // 服务器监听地址ip+port
               const string &nameArg)         // 服务器名字
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

    void start()
    {
        _server.start(); // 开启事件循环，监听事件，等待用户连接和请求
    }

private:
    void onConnection(const TcpConnectionPtr &conn) // 处理用户连接和断开回调函数
    {
        if (conn->connected())
        {
            cout << "用户连接:" << conn->peerAddress().toIpPort() << " ->" << conn->localAddress().toIpPort() << "state:online" << endl;
        }
        else
        {
            cout << "用户断开:" << conn->peerAddress().toIpPort() << " ->" << conn->localAddress().toIpPort() << "state:offline" << endl;
            conn->shutdown(); // 关闭连接，触发服务器的close回调函数，关闭套接字，释放资源
            //_loop->quit();    // 退出事件循环，结束服务器程序
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) // 处理用户读写事件回调函数
    {
        string recvBuf = buf->retrieveAllAsString(); // 从缓冲区中读取数据
        cout << "recv data:" << recvBuf << " time:" << time.toString() << endl;
        conn->send(recvBuf); // 将数据发送回客户端
    }

    // void onWriteComplete(const TcpConnectionPtr &);// 处理用户读写事件回调函数
    TcpServer _server;
    EventLoop *_loop;
};

int main()
{
    EventLoop loop;                                 // 创建事件循环对象
    InetAddress listenAddr("192.168.96.165", 8888); // 服务器监听地址ip+port
    ChatServer server(&loop, listenAddr, "ChatServer");
    server.start(); // 启动服务器，监听事件，等待用户连接和请求
    loop.loop();    // 启动事件循环，等待用户连接和请求 epoll_wait以阻塞的方式等待事件发生，处理事件，分发事件，继续等待事件发生
    return 0;
}