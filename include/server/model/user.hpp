#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// User表的ORM类
class User
{
public:
    User(int id = -1, const string &name = "", const string &password = "", const string &state = "offline")
    {
        this->_id = id;             // 初始化用户id
        this->_name = name;         // 初始化用户名称
        this->_password = password; // 初始化用户密码
        this->_state = state;       // 初始化用户状态，默认为离线
    }

    void setId(int id) { this->_id = id; }  // 设置用户id
    int getId() const { return this->_id; } // 获取用户id

    void setName(const string &name) { this->_name = name; } // 设置用户名称
    string getName() const { return this->_name; }           // 获取用户名称

    void setPassword(const string &password) { this->_password = password; } // 设置用户密码
    string getPassword() const { return this->_password; }                   // 获取用户密码

    void setState(const string &state) { this->_state = state; } // 设置用户状态
    string getState() const { return this->_state; }             // 获取用户状态

private:
    int _id;          // 用户id
    string _name;     // 用户名称
    string _password; // 用户密码
    string _state;    // 用户状态，在线、离线、忙碌等
};

#endif