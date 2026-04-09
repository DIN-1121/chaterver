cmake_minimum_required(VERSION 3.22.1)
project(main)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#配置头文件搜索路径
#include_directories(/home/din/muduo/muduo/net)

#配置库文件搜索路径
#link_directories(/home/din/muduo/build/lib)

#设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

#设置可执行文件最终生成的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#将当前目录下的所有源文件添加到源文件列表中
#aux_source_directory(.SRC_LIST)

#添加可执行文件
add_executable(server ${SRC_LIST})

#链接库文件
target_link_libraries(server muduo_net muduo_base pthread)