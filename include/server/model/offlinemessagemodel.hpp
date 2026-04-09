#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <vector>
#include <string>
using namespace std;

// 离线消息数据操作类
class OfflineMsgModel
{
    public:
        void insert(int userid, string msg); // 存储用户的离线消息
        vector<string> query(int userid);     // 查询用户的离线消息
        void remove(int userid);              // 删除用户的离线消息
};


#endif