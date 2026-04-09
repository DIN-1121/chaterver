#include "UserModel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// 插入用户数据
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')", user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection())); // 获取插入数据的id，并设置到用户对象中
            return true;
        }
    }
    return false;
}

// 根据用户id查询用户信息
User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id); // 根据主键查询，效率高，返回结果唯一

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            MYSQL_ROW row = mysql_fetch_row(res); // 从结果集中获取下一行数据，返回一个字符串数组，数组中的每个元素对应一列数据
            if (row)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); // 释放结果集，避免内存泄漏
                return user;
            }
        }
    }
    return User(); // 如果查询失败，返回一个默认的用户对象
}

// 更新用户状态
bool UserModel::updateState(const User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

/*// 重置用户状态，服务器异常退出后调用，确保所有用户状态为offline
void UserModel::resetState()
{
    // 1.组装sql语句
    char sql[1024] = "update User set state = 'offline' where state != 'offline'"; // 将所有不为offline状态的用户状态设置为offline

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql); // 执行SQL语句，重置用户状态到数据库中
    }
}*/