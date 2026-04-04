cmake_minimum_required(VERSION 3.5)

# 设置项目名称
project(Tpr20Library)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include
        )

# 查找源文件
set(SOURCES src/libmanager.cpp
            src/coordinatetrans.cpp
            src/libcrobot.cpp)

# 创建共享库 (.so)
add_library(tpr20pro SHARED ${SOURCES})

# 设置共享库的输出路径（可选）
set_target_properties(tpr20pro PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
