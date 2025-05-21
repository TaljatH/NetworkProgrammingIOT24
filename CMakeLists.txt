cmake_minimum_required(VERSION 3.10)
project(RobustServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(robust_server robust.server.cpp)
target_link_libraries(robust_server pthread)