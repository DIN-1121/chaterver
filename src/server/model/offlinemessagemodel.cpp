#include "offlinemessagemodel.hpp"
#include "mysqldb/db.h"

// 实现存储离线消息的逻辑
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};                                                             // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "insert into OfflineMessage values(%d, '%s')", userid, msg.c_str()); // 格式化SQL语句，将用户id和消息内容插入到离线消息表中
    MySQL mysql;                                                                      // 创建MySQL对象
    if (mysql.connect())                                                              // 连接数据库，如果连接成功，执行SQL语句
    {
        mysql.update(sql); // 执行SQL语句，插入离线消息到数据库中
    }
}

// 实现查询离线消息的逻辑
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};                                                       // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "select message from OfflineMessage where userid=%d", userid); // 格式化SQL语句，根据用户id查询离线消息
    MySQL mysql;                                                                // 创建MySQL对象
    vector<string> vec;                                                         // 定义一个字符串向量，用于存储查询结果
    if (mysql.connect())                                                        // 连接数据库，如果连接成功，执行SQL语句
    {
        MYSQL_RES *res = mysql.query(sql); // 执行SQL语句，查询数据库中的离线消息，返回结果集
        if (res != nullptr)                // 如果结果集不为空，遍历结果集，将每条离线消息添加到字符串向量中
        {
            MYSQL_ROW row;                                  // 定义一个MYSQL_ROW类型的变量，用于存储每一行的查询结果
            while ((row = mysql_fetch_row(res)) != nullptr) // 循环获取结果集中的每一行，直到没有更多的行
            {
                vec.push_back(row[0]); // 将查询结果中的消息内容添加到字符串向量中，row[0]表示第一列，即消息内容
            }
            mysql_free_result(res); // 释放结果集占用的内存
        }
    }
    return vec; // 返回存储离线消息的字符串向量
}

// 实现删除离线消息的逻辑
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};                                               // 定义一个字符数组，用于存储SQL语句
    sprintf(sql, "delete from OfflineMessage where userid=%d", userid); // 格式化SQL语句，根据用户id删除离线消息
    MySQL mysql;                                                        // 创建MySQL对象
    if (mysql.connect())                                                // 连接数据库，如果连接成功，执行SQL语句
    {
        mysql.update(sql); // 执行SQL语句，删除数据库中的离线消息
    }
}