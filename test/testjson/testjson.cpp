#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

// json序列化
void func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello, lisi";

    string sendBuf = js.dump();
    cout << sendBuf.c_str() << endl;
    cout << js << endl;
}
// json序列化2
void func2()
{
    json js;
    // 添加数组
    js["id"] = {1, 2, 3, 4, 5};
    // 添加key-value
    js["name"] = "zhangsan";
    // 添加对象
    js["msg"]["zhangsan"] = "hello, lisi";
    js["msg"]["lisi"] = "hello, zhangsan";
    cout << js << endl;
    // 与上等同
    js["msg"] = {{"zhangsan", "hello, lisi"}, {"lisi", "hello, zhangsan"}};
    cout << js << endl;
}
// json序列化3
void func3()
{
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "太少"});
    m.insert({3, "华山"});

    js["path"] = m;

    string sendBuf = js.dump(); // 将json对象序列化成字符串
    cout << sendBuf.c_str() << endl;
    cout << sendBuf << endl;
}
// json反序列化1
void func4()
{
    string recvBuf = R"({"msg_type":2,"from":"zhangsan","to":"lisi","msg":"hello, lisi"})";
    json js = json::parse(recvBuf);
    cout << js["msg_type"] << endl;
    cout << js["from"] << endl;
    cout << js["to"] << endl;
    cout << js["msg"] << endl;
}
int main()
{
    func4();
    return 0;
}