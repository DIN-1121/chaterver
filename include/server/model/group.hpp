#ifndef GROUP_H
#define GROUP_H

#include "user.hpp"
#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// 群组表的ORM类
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "") : _id(id), _name(name), _desc(desc) {} // 构造函数，初始化群组id和名称

    void setId(int id) { this->_id = id; }  // 设置群组id
    int getId() const { return this->_id; } // 获取群组id

    void setName(const string &name) { this->_name = name; } // 设置群组名称
    string getName() const { return this->_name; }           // 获取群组名称

    void setDesc(const string &desc) { this->_desc = desc; } // 设置群组描述
    string getDesc() const { return this->_desc; }           // 获取群组描述

    vector<GroupUser> &getUsers()  { return this->users; } // 获取群组成员列表
private:
    int _id;                 // 群组id
    string _name;            // 群组名称
    string _desc;            // 群组描述
    vector<GroupUser> users; // 群组成员列表，存储群组成员的用户id和角色信息
};

#endif