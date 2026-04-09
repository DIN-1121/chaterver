#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
{
    _publish_context = nullptr;   // 初始化发布消息的上下文对象为nullptr
    _subscribe_context = nullptr; // 初始化订阅消息的上下文对象为nullptr
}

// 连接redis服务器
bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379); // 连接redis服务器，指定服务器地址和端口号 负责发布消息的上下文对象
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed!" << endl; // 输出错误信息，表示连接redis服务器失败
        return false;
    }

    _subscribe_context = redisConnect("127.0.0.1", 6379); // 连接redis服务器，指定服务器地址和端口号 负责订阅消息的上下文对象
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed!" << endl; // 输出错误信息，表示连接redis服务器失败
        return false;
    }

    thread t([&]()
             {
                 observer_channel_message(); // 启动一个线程，调用消息处理函数，监听订阅的消息频道，处理接收到的消息
             });
    t.detach();  // 将线程分离，允许它在后台运行
    return true; // 返回连接redis服务器成功
}

// 向指定频道发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str()); // 向指定频道发布消息，使用redisCommand函数执行PUBLISH命令，传入频道和消息内容
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl; // 输出错误信息，表示发布消息失败
        return false;
    }
    freeReplyObject(reply); // 释放redisReply对象占用的内存
    return true;            // 返回发布消息成功
}

// 向指定频道订阅消息
bool Redis::subscribe(int channel)
{
    /*
    SUBSCRIBE命令是一个阻塞命令，执行后会一直等待服务器推送消息，直到客户端取消订阅或者连接断开。
    因此，不能直接使用redisCommand函数执行SUBSCRIBE命令，否则会导致当前线程被阻塞，无法继续执行后续的代码。
    通道消息的接收和处理应该在一个独立的线程中进行，使用redisAppendCommand函数将SUBSCRIBE命令添加到订阅上下文的命令队列中，然后使用redisBufferWrite函数将命令发送到服务器，最后在独立的线程中调用observer_channel_message函数监听订阅的消息频道，处理接收到的消息。
    只负责发送命令，不阻塞redis server响应消息，否则会和notify_message_handler回调函数冲突 抢占资源，导致无法接收消息
    */
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel)) // 向指定频道订阅消息，使用redisAppendCommand函数将SUBSCRIBE命令添加到订阅上下文的命令队列中，传入频道参数
    {
        cerr << "subscribe command failed!" << endl; // 输出错误信息，表示订阅消息失败
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) // 将订阅命令发送到redis服务器，使用redisBufferWrite函数将命令缓冲区中的命令发送到服务器，传入订阅上下文和done参数，done参数用于指示是否所有命令都已发送
        {
            cerr << "subscribe command failed!" << endl; // 输出错误信息，表示订阅消息失败
            return false;
        }
    }
    return true; // 返回订阅消息成功
}

// 向指定频道取消订阅
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel)) // 向指定频道取消订阅，使用redisAppendCommand函数将UNSUBSCRIBE命令添加到订阅上下文的命令队列中，传入频道参数
    {
        cerr << "unsubscribe command failed!" << endl; // 输出错误信息，表示取消订阅失败
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) // 将取消订阅命令发送到redis服务器，使用redisBufferWrite函数将命令缓冲区中的命令发送到服务器，传入订阅上下文和done参数，done参数用于指示是否所有命令都已发送
        {
            cerr << "unsubscribe command failed!" << endl; // 输出错误信息，表示取消订阅失败
            return false;
        }
    }
    return true; // 返回取消订阅成功
}

// 独立线程消息处理函数
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;                                                 // 定义一个redisReply指针，用于存储从redis服务器接收到的消息
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply)) // 循环监听订阅的消息频道，使用redisGetReply函数从订阅上下文中获取服务器推送的消息，传入订阅上下文和reply参数，返回值为REDIS_OK表示成功获取到消息
    {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr) // 判断获取到的消息是否有效，检查reply对象和消息内容是否为空
        {
            string channel(reply->element[1]->str);          // 获取消息频道，使用reply对象的element成员访问消息频道信息，element[1]表示频道信息，转换为string类型
            string message(reply->element[2]->str);          // 获取消息内容，使用reply对象的element成员访问消息内容信息，element[2]表示消息内容信息，转换为string类型
            _notify_message_handler(stoi(channel), message); // 调用消息处理函数对象，将频道和消息内容作为参数传入，处理接收到的消息
        }
        freeReplyObject(reply); // 释放redisReply对象占用的内存
    }
    cerr << "observer_channel_message error!" << endl; // 输出错误信息，表示监听消息频道失败
}

// 初始化消息处理函数对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn; // 将传入的函数对象赋值给成员变量_notify_message_handler，初始化消息处理函数对象
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context); // 释放发布消息的上下文对象
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context); // 释放订阅消息的上下文对象
    }
}