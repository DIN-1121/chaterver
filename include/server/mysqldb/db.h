#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <muduo/net/TcpConnection.h>
#include <string>
using namespace std;

// 封装MySQL操作的类
class MySQL
{
public:
    MySQL();  // 构造函数，初始化MySQL连接对象
    ~MySQL(); // 析构函数，关闭MySQL连接

    // 连接数据库
    bool connect();

    // 更新操作，增删改
    bool update(string sql);

    // 查询操作，返回结果集
    MYSQL_RES *query(string sql);

    // 获取MySQL连接对象
    MYSQL *getConnection();

private:
    MYSQL *_conn; // MySQL连接对象
};

#endif