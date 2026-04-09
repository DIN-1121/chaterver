#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

using namespace std;
using json = nlohmann::json; // 使用nlohmann库的json命名

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前系统登录的用户的好友列表信息
vector<User> g_friendList;
// 记录当前系统登录的用户的群组列表信息
vector<Group> g_groupList;
// 控制聊天系统的主界面显示
bool isMainMenuRunning = false;
// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool isLoginSuccess{false};

// 显示当前登录用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息的时间戳）
string getCurrentTime();
// 主聊天界面
void mainMenu(int clientfd);

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <IP_ADDRESS> <PORT>" << endl;
        return -1;
    }

    string ip = argv[1];           // 获取服务器IP地址
    uint16_t port = atoi(argv[2]); // 获取服务器端口号

    // 创建客户端socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket error" << endl;
        return -1;
    }

    sem_init(&rwsem, 0, 0); // 初始化读写线程之间的通信信号量

    // 连接服务器,将服务器地址和端口号封装到sockaddr_in结构体中，并调用connect函数连接服务器
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(clientfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        cerr << "connect error" << endl;
        return -1;
    }

    std::thread readTask(readTaskHandler, clientfd); // 启动接收线程，处理服务器发送的消息
    readTask.detach();                               // 将接收线程分离，允许它在后台运行

    // 连接成功，启动接收线程，显示主菜单，处理用户输入
    for (;;)
    {
        cout << "=================欢迎进入聊天系统=================" << endl;
        cout << "1.登录" << endl;
        cout << "2.注册" << endl;
        cout << "3.退出" << endl;
        cout << "请选择操作：";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读取输入缓冲区中的换行符，避免后续输入被跳过
        switch (choice)
        {
        case 1: // 登录业务
        {
            int id;
            char password[50];
            cout << "请输入用户id：";
            cin >> id;
            cin.get(); // 读取输入缓冲区中的换行符，避免后续输入被跳过
            cout << "请输入用户密码：";
            cin.getline(password, 50); // 使用getline读取密码，允许密码中包含空格
            
                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = password;
                string request = js.dump();

                isLoginSuccess = false; // 重置登录状态，确保每次登录尝试都能正确处理登录响应

                int len = send(clientfd, request.c_str(), request.size(), 0);
                if (len == -1)
                {
                    cerr << "send error" << endl;
                }
                sem_wait(&rwsem);   // 等待登录响应消息被处理完，确保登录状态和用户信息已经更新
                if (isLoginSuccess) // 根据登录状态决定是否显示主菜单
                {
                    isMainMenuRunning = true; // 设置主菜单运行标志，控制主菜单的显示
                    mainMenu(clientfd);       // 显示主菜单，处理用户输入
                }
            }
            break;
        case 2: // 注册业务
        {
            char name[50];
            char password[50];
            cout << "请输入用户名：";
            cin.getline(name, 50); // 使用getline读取用户名，允许用户名中包含空格
            cout << "请输入用户密码：";
            cin.getline(password, 50); // 使用getline读取密码，允许密码中包含空格
            
                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;
                string request = js.dump();

                

                int len = send(clientfd, request.c_str(), request.size(), 0);
                if (len == -1)
                {
                    cerr << "send error" << endl;
                }
                sem_wait(&rwsem); // 等待注册响应消息被处理完，确保注册结果已经显示给用户
        }
            break;
        case 3: // 退出业务
            cout << "感谢使用聊天系统，再见！" << endl;
            close(clientfd);
            sem_destroy(&rwsem); // 销毁读写线程之间的通信信号量
            exit(0);
        default:
            cerr << "无效的选择，请重新输入！" << endl;
            break;
        }
    }
    // close(clientfd);
    return 0;
}

void doLoginResponse(json &response) // 处理登录响应消息，更新登录状态和用户信息
{
    if (response["errno"].get<int>() == 0)
    {
        cout << "登录成功！" << endl;
        g_currentUser.setId(response["id"].get<int>());        // 设置当前用户的id
        g_currentUser.setName(response["name"].get<string>()); // 设置当前用户的名称

        g_friendList.clear(); // 初始化好友列表
        g_groupList.clear();  // 初始化群组列表

        if (response.contains("friends"))
        {

            vector<string> friendVecStr = response["friends"].get<vector<string>>(); // 获取好友列表的字符串表示
            for (const string &friendStr : friendVecStr)                             // 遍历好友列表的字符串表示，将每个好友的用户信息解析出来，并添加到当前用户的好友列表中
            {
                json friendJson = json::parse(friendStr);               // 将好友字符串解析为json对象
                User friendUser;                                        // 创建一个用户对象，用于存储好友的用户信息
                friendUser.setId(friendJson["id"].get<int>());          // 设置好友的用户id
                friendUser.setName(friendJson["name"].get<string>());   // 设置好友的用户名
                friendUser.setState(friendJson["state"].get<string>()); // 设置好友的用户状态
                g_friendList.push_back(friendUser);                     // 将好友用户对象添加到当前用户的好友列表中
            }
        }

        if (response.contains("groups"))
        {

            vector<string> groupVecStr = response["groups"].get<vector<string>>(); // 获取群组列表的字符串表示
            for (const string &groupStr : groupVecStr)                             // 遍历群组列表的字符串表示，将每个群组的信息解析出来，并添加到当前用户的群组列表中
            {
                json groupJson = json::parse(groupStr);         // 将群组字符串解析为json对象
                Group group;                                    // 创建一个群组对象，用于存储群组的信息
                group.setId(groupJson["id"].get<int>());        // 设置群组id
                group.setName(groupJson["name"].get<string>()); // 设置群组名称
                group.setDesc(groupJson["desc"].get<string>()); // 设置群组描述
                // g_groupList.push_back(group);                   // 将群组对象添加到当前用户的群组列表中

                if (groupJson.contains("users"))
                {
                    vector<string> groupUserVecStr = groupJson["users"].get<vector<string>>(); // 获取群组成员列表的字符串表示
                    for (const string &groupUserStr : groupUserVecStr)                         // 遍历群组成员列表的字符串表示，将每个群组成员的信息解析出来，并添加到当前群组的成员列表中
                    {
                        json groupUserJson = json::parse(groupUserStr);           // 将群组成员字符串解析为json对象
                        GroupUser groupUser;                                      // 创建一个群组成员对象，用于存储群组成员的信息
                        groupUser.setId(groupUserJson["id"].get<int>());          // 设置群组成员的用户id
                        groupUser.setName(groupUserJson["name"].get<string>());   // 设置群组成员的用户名
                        groupUser.setState(groupUserJson["state"].get<string>()); // 设置群组成员的用户状态
                        groupUser.setRole(groupUserJson["role"].get<string>());   // 设置群组成员的角色
                        group.getUsers().push_back(groupUser);                    // 将群组成员对象添加到当前群组的成员列表中
                    }
                }
                g_groupList.push_back(group); // 将群组对象添加到当前用户的群组列表中
            }
        }

        showCurrentUserData(); // 显示当前登录用户的基本信息

        if (response.contains("offlinemsg"))
        {
            vector<string> offlineMsgVec = response["offlinemsg"].get<vector<string>>(); // 获取离线消息列表，返回一个字符串向量
            for (const string &offlineMsgStr : offlineMsgVec)
            {
                json msgJson = json::parse(offlineMsgStr);
                int msgid = msgJson.value("msgid", -1);
                if (msgid == ONE_CHAT_MSG)
                {
                    // 一对一聊天消息
                    string name = msgJson.value("name", "未知");
                    int id = msgJson.value("id", 0);
                    string message = msgJson.value("message", "");
                    string time = msgJson.value("time", "");
                    cout << "离线消息（好友）: " << name << "(" << id << ") 说: " << message << " [" << time << "]" << endl;
                }
                else if (msgid == GROUP_CHAT_MSG)
                {
                    // 群聊消息
                    string name = msgJson.value("name", "未知");
                    int id = msgJson.value("id", 0);
                    int groupid = msgJson.value("groupid", 0);
                    string message = msgJson.value("message", "");
                    string time = msgJson.value("time", "");
                    cout << "离线消息（群聊[" << groupid << "]）: " << name << "(" << id << ") 说: " << message << " [" << time << "]" << endl;
                }
                else
                {
                    // 其他类型，打印原始字符串
                    cout << "离线消息（其他）: " << offlineMsgStr << endl;
                }
            }
        }
        isLoginSuccess = true; // 设置登录状态为true，表示登录成功
    }
    else
    {
        cerr << "登录失败" << endl;
        isLoginSuccess = false; // 设置登录状态为false，表示登录失败
    }
}

// 处理注册响应消息，显示注册结果
void doRegResponse(const json &response)
{
    if (response["errno"].get<int>() == 0)
    {
        cout << "注册成功，您的用户id是：" << response["id"].get<int>() << endl;
        cout << "请牢记您的用户id，登录时需要使用！" << endl;
    }
    else
    {
        string errmsg = response.value("errmsg", "该用户已存在!!!");
        cout << "注册失败：" << errmsg << endl;
    }
}

// 显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout << "============login user==============" << endl;
    cout << "用户名：" << g_currentUser.getName() << endl;
    cout << "===================friend list==================" << endl;
    if (g_friendList.empty())
    {
        cout << "您还没有好友，快去添加好友吧！" << endl;
    }
    else
    {
        for (User &user : g_friendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "===================group list==================" << endl;
    if (g_groupList.empty())
    {
        cout << "您还没有加入任何群组，快去加入群组吧！" << endl;
    }
    else
    {
        for (Group &group : g_groupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << "    " << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "=====================================" << endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);
        if (len == -1 || len == 0)
        {
            cerr << "recv error or server closed connection" << endl;
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        // 安全获取 msgid
        if (!js.contains("msgid") || !js["msgid"].is_number())
        {
            cerr << "收到无效消息（缺少 msgid 或类型错误）: " << js.dump() << endl;
            continue;
        }
        int msgid = js["msgid"];
        switch (msgid)
        {
        case ONE_CHAT_MSG:
            cout << js["id"] << ":" << js["message"] << endl;
            break;
        case GROUP_CHAT_MSG:
            cout << "群：" << js["groupid"] << ": " << js["id"] << ":" << js["message"] << endl;
            break;
        case LOGIN_MSG_ACK:
            doLoginResponse(js); // 处理登录响应消息，更新登录状态和用户信息
            sem_post(&rwsem);    // 通知主线程登录响应已处理完，可以继续执行主线程的登录流程
            break;
        case REG_MSG_ACK:
            doRegResponse(js); // 处理注册响应消息，显示注册结果
            sem_post(&rwsem);  // 通知主线程注册响应已处理完，可以继续执行主线程的注册流程
            break;
        default:
            break;
        }
    }
}
// 获取系统时间（聊天信息的时间戳）
string getCurrentTime()
{
    auto now = chrono::system_clock::now();                                          // 获取当前系统时间点
    time_t now_time_t = chrono::system_clock::to_time_t(now);                        // 将时间点转换为time_t类型
    char timeStr[100] = {0};                                                         // 定义一个字符数组，用于存储格式化后的时间字符串
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now_time_t)); // 将time_t类型的时间转换为本地时间，并格式化为字符串，格式为"年-月-日 时:分:秒"
    return string(timeStr);                                                          // 返回格式化后的时间字符串
}

// 系统支持的客户端命令和对应的处理函数
unordered_map<string, string> commandMap = { // 定义一个unordered_map，存储系统支持的客户端命令和对应的命令描述信息
    {"help", "显示帮助信息，列出系统支持的客户端命令"},
    {"chat", "单人聊天，消息格式：chat:friendid:message"},
    {"addfriend", "添加好友，消息格式：addfriend:friendid"},
    {"creategroup", "创建群组，消息格式：creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，消息格式：addgroup:groupid"},
    {"groupchat", "群聊，消息格式：groupchat:groupid:message"},
    {"loginout", "退出登录，消息格式：loginout"}};

void help(int clientfd = 0, string str = "") // 显示帮助信息，列出系统支持的客户端命令
{
    cout << "系统支持的命令如下：" << endl;
    for (const auto &command : commandMap) // 遍历系统支持的客户端命令映射表，显示每个命令和对应的描述信息
    {
        cout << command.first << ": " << command.second << endl;
    }
}

void chat(int clientfd, const string &command) // 处理单人聊天命令
{
    // 解析命令，提取好友id和消息内容
    int idx1 = command.find(":");
    int idx2 = command.find(":", idx1 + 1);
    if (idx1 == -1 || idx2 == -1)
    {
        cout << "无效的命令格式，请使用：chat:friendid:message" << endl;
        return;
    }
    int friendId = stoi(command.substr(idx1 + 1, idx2 - idx1 - 1)); // 从命令字符串中解析出好友id
    string message = command.substr(idx2 + 1);                      // 从命令字符串中解析出消息内容

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["friendid"] = friendId;
    js["message"] = message;
    js["time"] = getCurrentTime();

    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
}

void addFriend(int clientfd, const string &command) // 处理添加好友命令
{
    // 解析命令，提取好友id
    int idx = command.find(":");
    if (idx == -1)
    {
        cout << "无效的命令格式，请使用：addfriend:friendid" << endl;
        return;
    }
    int friendId = stoi(command.substr(idx + 1)); // 从命令字符串中解析出好友id

    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendId;

    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
}

void createGroup(int clientfd, const string &command) // 处理创建群组命令
{
    // 解析命令，提取群组名称和群组描述
    int idx1 = command.find(":");
    int idx2 = command.find(":", idx1 + 1);
    if (idx1 == -1 || idx2 == -1)
    {
        cout << "无效的命令格式，请使用：creategroup:groupname:groupdesc" << endl;
        return;
    }
    string groupName = command.substr(idx1 + 1, idx2 - idx1 - 1); // 从命令字符串中解析出群组名称
    string groupDesc = command.substr(idx2 + 1);                  // 从命令字符串中解析出群组描述

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupName;
    js["groupdesc"] = groupDesc;

    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
}

void addGroup(int clientfd, const string &command) // 处理加入群组命令
{
    // 解析命令，提取群组id
    int idx = command.find(":");
    if (idx == -1)
    {
        cout << "无效的命令格式，请使用：addgroup:groupid" << endl;
        return;
    }
    int groupId = stoi(command.substr(idx + 1)); // 从命令字符串中解析出群组id

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupId;

    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
}

void groupChat(int clientfd, const string &command) // 处理群聊命令
{
    // 解析命令，提取群组id和消息内容
    int idx1 = command.find(":");
    int idx2 = command.find(":", idx1 + 1);
    if (idx1 == -1 || idx2 == -1)
    {
        cout << "无效的命令格式，请使用：groupchat:groupid:message" << endl;
        return;
    }
    int groupId = stoi(command.substr(idx1 + 1, idx2 - idx1 - 1)); // 从命令字符串中解析出群组id
    string message = command.substr(idx2 + 1);                     // 从命令字符串中解析出消息内容

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupId;
    js["message"] = message;
    js["time"] = getCurrentTime();

    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
}

void loginout(int clientfd, const string &command) // 处理退出登录命令
{
    cout << "感谢使用，再见！" << endl;
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string request = js.dump();
    int len = send(clientfd, request.c_str(), request.size(), 0); // 将消息发送给服务器，通知服务器当前用户退出登录
    if (len == -1)
    {
        cerr << "send error" << endl;
    }
    isMainMenuRunning = false; // 设置主菜单运行标志为false，控制主菜单的显示
}

// 注册系统支持的客户端命令
unordered_map<string, function<void(int, string)>> commandHandlerMap = { // 定义一个unordered_map，存储系统支持的客户端命令和对应的处理函数
    {"help", help},
    {"chat", chat},
    {"addfriend", addFriend},
    {"creategroup", createGroup},
    {"addgroup", addGroup},
    {"groupchat", groupChat},
    {"loginout", loginout}};

// 主聊天界面
void mainMenu(int clientfd)
{
    help(); // 显示帮助信息，列出系统支持的客户端命令

    char buffer[1024] = {0}; // 定义一个字符数组，用于存储用户输入的命令
    while (isMainMenuRunning)
    {
        cin.getline(buffer, sizeof(buffer)); // 读取用户输入的命令，存储到buffer中
        string commandbuf(buffer);           // 将用户输入的命令转换为string类型，方便后续处理
        string command;                      // 定义一个字符串变量，用于存储解析后的命令
        int idx = commandbuf.find(":");      // 查找命令字符串中第一个冒号的位置，用于解析命令
        if (idx != -1)
        {
            command = commandbuf.substr(0, idx); // 从命令字符串中解析出命令部分，即冒号前面的内容
        }
        else
        {
            command = commandbuf; // 如果命令字符串中没有冒号，则整个字符串就是命令
        }
        auto it = commandHandlerMap.find(command); // 在系统支持的客户端命令映射表中查找用户输入的命令
        if (it != commandHandlerMap.end())
        {
            it->second(clientfd, commandbuf); // 如果找到对应的命令处理函数，则调用该处理函数，传入客户端socket和用户输入的命令字符串
        }
        else
        {
            cout << "无效的命令，请重新输入！" << endl; // 如果用户输入的命令在系统支持的客户端命令映射表中没有找到，则提示用户输入了无效的命令
        }
    }
}
