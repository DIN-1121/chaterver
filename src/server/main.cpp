#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器退出信号
void resetHandler(int)
{
    ChatService::instance()->reset(); // 调用聊天服务器的重置函数，清理服务器资源和状态
    exit(0);                          // 退出程序
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 8889" << endl;
        exit(1);
    }

    char *ip = argv[1];            // 获取服务器IP地址
    uint16_t port = atoi(argv[2]); // 获取服务器端口号

    signal(SIGINT, resetHandler); // 注册信号处理函数，处理服务器退出信号

    EventLoop loop;                                     // 创建事件循环对象
    InetAddress listenAddr(ip, port);                 // 创建服务器监听地址对象，指定端口号
    ChatServer server(&loop, listenAddr, "ChatServer"); // 创建聊天服务器对象，传入事件循环对象指针、监听地址和服务器名字
    server.start();                                     // 启动服务器
    loop.loop();                                        // 进入事件循环，等待用户连接和请求
    return 0;
}