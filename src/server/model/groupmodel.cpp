#include "groupmodel.hpp"
#include <iostream>
using namespace std;

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};                                                                                                              // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "insert into ALLGroup( groupname , groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str()); // 格式化SQL语句，将群组名称和描述插入到群组表中
    MySQL mysql;                                                                                                                       // 创建MySQL对象
    if (mysql.connect())                                                                                                               // 连接数据库，如果连接成功，执行SQL语句
    {
        if (mysql.update(sql)) // 执行SQL语句，插入群组到数据库中
        {
            group.setId(mysql_insert_id(mysql.getConnection())); // 获取插入数据的id，并设置到群组对象中
            return true;                                         // 返回创建群组成功
        }
    }
    return false; // 返回创建群组失败
}

// 添加用户到群组，指定用户在群组中的角色
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};                                                                      // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "insert into GroupUser values(%d, %d, '%s')", groupid, userid, role.c_str()); // 格式化SQL语句，将群组id、用户id和角色插入到群组用户表中
    MySQL mysql;                                                                               // 创建MySQL对象
    if (mysql.connect())                                                                       // 连接数据库，如果连接成功，执行SQL语句
    {
        return mysql.update(sql); // 执行SQL语句，插入群组用户关系到数据库中，返回插入结果
    }
    return false; // 返回添加用户到群组失败
}

// 查询用户所在的群组列表，包含群组成员信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};                                                                                                                        // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "select g.id, g.groupname, g.groupdesc from ALLGroup g inner join GroupUser gu on gu.groupid=g.id where gu.userid=%d", userid); // 格式化SQL语句，根据用户id查询用户所在的群组列表，使用内连接查询群组表和群组用户表，返回群组的id、名称和描述
    MySQL mysql;                                                                                                                                 // 创建MySQL对象
    vector<Group> vec;                                                                                                                           // 定义一个群组对象向量，用于存储查询结果
    if (mysql.connect())                                                                                                                         // 连接数据库，如果连接成功，执行SQL语句
    {
        MYSQL_RES *res = mysql.query(sql); // 执行SQL语句，查询数据库中的群组列表，返回结果集
        if (res != nullptr)                // 如果结果集不为空，遍历结果集，将每个群组的信息添加到群组对象向量中
        {
            MYSQL_ROW row; // 定义一个MYSQL_ROW类型的变量，用于存储每一行的查询结果
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);

                // 一条 SQL 获取群组成员详细信息（id, name, state, role）
                char sql2[1024] = {0};
                sprintf(sql2, "SELECT u.id, u.name, u.state, gu.grouprole FROM User u JOIN GroupUser gu ON u.id = gu.userid WHERE gu.groupid=%d", group.getId());
                MySQL mysql2;
                if (mysql2.connect())
                {
                    MYSQL_RES *res2 = mysql2.query(sql2);
                    if (res2)
                    {
                        MYSQL_ROW row2;
                        while ((row2 = mysql_fetch_row(res2)))
                        {
                            GroupUser gu;
                            gu.setId(atoi(row2[0]));
                            gu.setName(row2[1]);
                            gu.setState(row2[2]);
                            gu.setRole(row2[3]); // 直接获得角色
                            group.getUsers().push_back(gu);
                        }
                        mysql_free_result(res2);
                    }
                }
                vec.push_back(group);
            }
            mysql_free_result(res); // 释放结果集占用的内存
        }
    }
    return vec; // 返回存储用户所在的群组列表的群组对象向量
}

// 查询群组用户id列表，返回用户id和角色信息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};                                                              // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "select userid, grouprole from GroupUser where groupid=%d", groupid); // 格式化SQL语句，根据群组id查询群组用户列表，返回用户id和角色信息
    MySQL mysql;                                                                       // 创建MySQL对象
    vector<int> vec;                                                                   // 定义一个整数向量，用于存储查询结果
    if (mysql.connect())                                                               // 连接数据库，如果连接成功，执行SQL语句
    {
        MYSQL_RES *res = mysql.query(sql); // 执行SQL语句，查询数据库中的群组用户列表，返回结果集
        if (res != nullptr)                // 如果结果集不为空，遍历结果集，将每个群组用户的用户id和角色信息添加到整数向量中
        {
            MYSQL_ROW row;                                  // 定义一个MYSQL_ROW类型的变量，用于存储每一行的查询结果
            while ((row = mysql_fetch_row(res)) != nullptr) // 循环获取结果集中的每一行，直到没有更多的行
            {
                int id = atoi(row[0]); // 将查询结果中的用户id转换为整数，row[0]表示第一列，即用户id
                string role = row[1];  // 将查询结果中的角色信息设置到字符串变量中，row[1]表示第二列，即角色信息
                vec.push_back(id);     // 将用户id添加到整数向量中，返回给调用者使用
            }
            mysql_free_result(res); // 释放结果集占用的内存
        }
    }
    return vec; // 返回存储群组用户id列表的整数向量
}