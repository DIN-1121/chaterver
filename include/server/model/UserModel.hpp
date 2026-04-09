#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
#include "db.h"
#include <vector>
using namespace std;

// UserModel类，负责User表的操作
class UserModel
{
public:
    bool insert(User &user);                    // 插入用户数据
    User query(int id);                         // 根据用户id查询用户信息
    bool updateState(const User &user);         // 更新用户状态
    vector<User> queryAllByState(string state); // 查询指定状态的用户列表
    // void resetState();                          // 重置用户状态，服务器异常退出后调用，确保所有用户状态为offline
private:
};

#endif // USERMODEL_H