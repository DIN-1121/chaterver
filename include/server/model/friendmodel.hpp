#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>
using namespace std;

// FriendModel类，负责好友关系表的操作
class FriendModel
{
public:
    void insert(int userid, int friendid); // 添加好友关系
    vector<User> query(int userid);        // 查询好友列表
};

#endif