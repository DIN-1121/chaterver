#ifndef PUBLIC_H
#define PUBLIC_H
/*
服务器和客户端的公共文件，定义一些公共的常量和数据类型
*/

enum EnMsgType
{
    LOGIN_MSG = 1,      // 登录消息
    LOGIN_MSG_ACK,      // 登录响应消息
    REG_MSG = 2,        // 注册消息
    REG_MSG_ACK,        // 注册响应消息
    ONE_CHAT_MSG = 5,   // 单人聊天消息
    ADD_FRIEND_MSG = 6, // 添加好友消息

    CREATE_GROUP_MSG = 7, // 创建群组消息
    ADD_GROUP_MSG = 8,    // 加入群组消息
    GROUP_CHAT_MSG = 9,   // 群聊消息

    LOGINOUT_MSG = 10, // 注销消息
};

#endif