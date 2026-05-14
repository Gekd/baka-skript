cmake_minimum_required(VERSION 3.30)
project(estuav)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_package(PROJ REQUIRED)

add_subdirectory(extern/CLI11)
add_subdirectory(extern/json)

set(SOURCES
    src/main.cpp
    src/file.cpp
    src/timeparse.cpp
    src/dir.cpp
    src/csv.cpp
    src/wildnav.cpp
)

add_executable(estuav ${SOURCES})

target_include_directories(estuav PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(estuav PRIVATE
    ${OpenCV_LIBS}
    CURL::libcurl
    PROJ::proj
    CLI11::CLI11
    nlohmann_json::nlohmann_json
)