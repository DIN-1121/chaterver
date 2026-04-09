#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <string>
#include <functional>
#include <thread>
using namespace std;

/*
redis作为集群服务器通信基于发布-订阅消息队列
*/

class Redis
{
public:
    Redis();
    ~Redis();

    bool connect(); // 连接redis服务器

    bool publish(int channel, string message); // 发布消息

    bool subscribe(int channel); // 订阅消息

    bool unsubscribe(int channel); // 取消订阅

    void observer_channel_message(); // 消息处理函数

    void init_notify_handler(function<void(int, string)> fn); // 初始化消息处理函数对象

private:
    redisContext *_publish_context;                      // 发布消息的上下文对象
    redisContext *_subscribe_context;                    // 订阅消息的上下文对象
    function<void(int, string)> _notify_message_handler; // 消息处理函数对象
};

#endif