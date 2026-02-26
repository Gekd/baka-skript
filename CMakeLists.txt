cmake_minimum_required(VERSION 3.30)
project(skript)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Werror")

add_executable(skript src/main.cpp)
