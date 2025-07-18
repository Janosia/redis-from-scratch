cmake_minimum_required(VERSION 3.10)
project(redis-from-scratch CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
# Source files used by the server
set(SERVER_SOURCES
    ${PROJECT_SOURCE_DIR}/src/avl.cpp
    ${PROJECT_SOURCE_DIR}/src/hashtable.cpp
    ${PROJECT_SOURCE_DIR}/src/heap.cpp
    ${PROJECT_SOURCE_DIR}/src/server_helper.cpp
    ${PROJECT_SOURCE_DIR}/src/thread_pool.cpp
    ${PROJECT_SOURCE_DIR}/src/timer.cpp
    ${PROJECT_SOURCE_DIR}/src/zset.cpp
    ${PROJECT_SOURCE_DIR}/src/zset_cmds.cpp
    ${PROJECT_SOURCE_DIR}/src/server.cpp
)

# Server executable
add_executable(server ${SERVER_SOURCES})

# Client executable (just client.cpp)
add_executable(client ${PROJECT_SOURCE_DIR}/src/client.cpp)

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/include/)
