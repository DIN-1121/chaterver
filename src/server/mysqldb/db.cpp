#include "db.h"
#include <iostream>
using namespace std;

// 数据库配置信息
static string server = "192.168.96.165"; // 数据库服务器地址
static string user = "root";             // 数据库用户名
static string password = "041121";       // 数据库密码
static string dbname = "chat";           // 数据库名称

/*
mysql连接不上问题：
核心原因：caching_sha2_password 需要 RSA 公钥
MySQL 8.0 默认的认证插件 caching_sha2_password 在通过网络（TCP/IP，如 127.0.0.1）传输密码时，为了安全，会使用 RSA 公钥对密码进行加密。
客户端（无论是命令行 mysql 还是你 C++ 代码中链接的 libmysqlclient）需要从服务器获取这个公钥。
命令行客户端 mysql：会自动请求公钥，并成功完成加密认证。
某些旧版本的 libmysqlclient（比如低于 8.0.3 的库）默认不会自动请求公钥，导致密码以明文方式尝试发送，而被服务器拒绝，连接失败。

为什么命令行登录一次后，代码就能连上了？
因为 MySQL 的 caching_sha2_password 有一个“缓存”机制：
当你用命令行 mysql -h 127.0.0.1 -u root -p 成功登录后，服务器会将该用户的认证信息（包括加密后的密码哈希）缓存在内存中。
随后你的 C++ 程序再次尝试连接时，虽然客户端可能没有正确发送加密密码，但服务器发现该用户的认证信息已经在缓存中（来自刚才命令行登录的成功记录），
于是允许使用更简单的认证流程（甚至跳过公钥加密步骤），从而意外地让连接成功。

这其实是一个 安全降级行为，是 caching_sha2_password 为了减少 RSA 计算开销而设计的状态缓存。但这也会带来你遇到的“先命令行登录一次，代码才能连上”的诡异现象。
*/

MySQL::MySQL()
{
    _conn = mysql_init(nullptr); // 初始化MySQL连接对象
    if (_conn == nullptr)
    {
        LOG_ERROR << "MySQL init error!"; // 记录错误日志，表示MySQL初始化失败
    }
}
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn); // 关闭MySQL连接
    }
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0); // 连接数据库，指定服务器地址、用户名、密码、数据库名称和端口号
    if (p == nullptr)
    {
        // cout << mysql_error(_conn) << endl;  // 输出MySQL连接错误信息
        LOG_ERROR << "MySQL connect error!"; // 记录错误日志，表示MySQL连接失败
        return false;
    }
    mysql_query(_conn, "set names utf8"); // 设置字符集为utf8，解决中文乱码问题
    return true;
}

// 更新操作，增删改
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str())) // 执行SQL语句，返回非0表示执行失败
    {
        LOG_ERROR << sql << " error!"; // 记录错误日志，表示SQL语句执行失败
        return false;
    }
    return true;
}

// 查询操作，返回结果集
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_ERROR << sql << " error!"; // 记录错误日志，表示SQL语句执行失败
        return nullptr;
    }
    return mysql_store_result(_conn); // 返回结果集
}

// 获取MySQL连接对象
MYSQL *MySQL::getConnection()
{
    return _conn; // 返回MySQL连接对象
}