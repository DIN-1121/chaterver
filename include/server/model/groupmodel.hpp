#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include "db.h"
#include "UserModel.hpp"
#include <vector>
#include <string>
using namespace std;

// GroupModel类，负责Group表的操作
class GroupModel
{
public:
    bool createGroup(Group &group);                       // 创建群组
    bool addGroup(int userid, int groupid, string role);  // 添加用户到群组，指定用户在群组中的角色
    vector<Group> queryGroups(int userid);                // 查询用户所在的群组列表，包含群组成员信息
    vector<int> queryGroupUsers(int userid, int groupid); // 查询群组用户id列表，返回用户id和角色信息
};

#endif // GROUPMODEL_H