cmake_minimum_required(VERSION 3.15)

project(gateway)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加子目录
add_subdirectory(echo)
add_subdirectory(gateway)

# 链接子目录生成的库
# target_link_libraries(app PRIVATE echo gateway)