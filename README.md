# chatserver
基于muduo使用redis发布-订阅在Ubuntu下用nginx tcp负载均衡实现集群聊天服务器和客户端

#编译运行

#cd build

#./active.sh

#./ChatServer 127.0.0.1 port 开启服务器 此port为nginx 设置服务器端口

#./ChatClient 127.0.0.1 port 开启客户端 此port为nginx stream监听port

#该项目需要配置环境如下：

nginx

mysql

redis
