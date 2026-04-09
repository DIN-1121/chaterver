#include "friendmodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};                                                // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "insert into Friend values(%d, %d)", userid, friendid); // 格式化SQL语句，将用户id和好友id插入到好友关系表中
    MySQL mysql;                                                         // 创建MySQL对象
    if (mysql.connect())                                                 // 连接数据库，如果连接成功，执行SQL语句
    {
        mysql.update(sql); // 执行SQL语句，插入好友关系到数据库中
    }
}

// 查询好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};                                                                                                                  // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "select u.id, u.name, u.password, u.state from User u inner join Friend f on f.friendid=u.id where f.userid=%d", userid); // 格式化SQL语句，根据用户id查询好友列表，使用内连接查询好友关系表和用户表，返回好友的用户信息
    MySQL mysql;                                                                                                                           // 创建MySQL对象
    vector<User> vec;                                                                                                                      // 定义一个用户对象向量，用于存储查询结果
    if (mysql.connect())                                                                                                                   // 连接数据库，如果连接成功，执行SQL语句
    {
        MYSQL_RES *res = mysql.query(sql); // 执行SQL语句，查询数据库中的好友列表，返回结果集
        if (res != nullptr)                // 如果结果集不为空，遍历结果集，将每个好友的用户信息添加到用户对象向量中
        {
            MYSQL_ROW row;                                  // 定义一个MYSQL_ROW类型的变量，用于存储每一行的查询结果
            while ((row = mysql_fetch_row(res)) != nullptr) // 循环获取结果集中的每一行，直到没有更多的行
            {
                User user;                // 定义一个用户对象，用于存储每个好友的用户信息
                user.setId(atoi(row[0])); // 将查询结果中的用户id设置到用户对象中，row[0]表示第一列，即用户id
                user.setName(row[1]);     // 将查询结果中的用户名设置到用户对象中，row[1]表示第二列，即用户名
                user.setPassword(row[2]); // 将查询结果中的密码设置到用户对象中，row[2]表示第三列，即密码
                user.setState(row[3]);    // 将查询结果中的状态设置到用户对象中，row[3]表示第四列，即状态
                vec.push_back(user);      // 将用户对象添加到用户对象向量中
            }
            mysql_free_result(res); // 释放结果集占用的内存
        }
    }
    return vec; // 返回存储好友列表的用户对象向量
}