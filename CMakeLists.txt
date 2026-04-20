cmake_minimum_required(VERSION 3.30)
project(skript)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_package(PROJ REQUIRED)

add_subdirectory(extern/CLI11)
add_subdirectory(extern/json)

set(SOURCES
    src/main.cpp
    src/skript/file.cpp
    src/skript/time.cpp
    src/skript/dir.cpp
    src/skript/csv.cpp
    src/skript/wildnav.cpp
)

add_executable(skript ${SOURCES})

target_include_directories(skript PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(skript PRIVATE
    ${OpenCV_LIBS}
    CURL::libcurl
    PROJ::proj
    CLI11::CLI11
    nlohmann_json::nlohmann_json
)