#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"
#include <string>
using namespace std;

// 群组用户表的ORM类
class GroupUser:public User
{
public:
    void setRole(string role) { this->_role = role; } // 设置用户在群组中的角色
    string getRole() const { return this->_role; }    // 获取用户在群组中的角色
private:
    string _role; // 用户在群组中的角色，创建者、管理员、普通成员等
};

#endif