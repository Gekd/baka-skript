cmake_minimum_required(VERSION 3.30)
project(skript)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Werror")

find_package(OpenCV QUIET)

if(OpenCV_FOUND)
    add_executable(skript src/main.cpp src/skript/file.cpp)
    target_link_libraries(skript ${OpenCV_LIBS})  
else()
    include(FetchContent)

    FetchContent_Declare(
        opencv
        GIT_REPOSITORY https://github.com/opencv/opencv.git
        GIT_TAG 4.13.0
    )
    FetchContent_MakeAvailable(opencv)
    add_executable(skript src/main.cpp src/skript/file.cpp)
        target_link_libraries(skript PRIVATE
        opencv_core
        opencv_imgproc
        opencv_highgui
    )
    target_include_directories(skript PRIVATE ${opencv_SOURCE_DIR}/include)
endif()
