cmake_minimum_required(VERSION 3.30)
project(skript)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Werror")
find_package( OpenCV REQUIRED )                              

add_executable(skript src/main.cpp src/skript/file.cpp)
target_link_libraries(skript ${OpenCV_LIBS})  