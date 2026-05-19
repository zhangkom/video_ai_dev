cmake_minimum_required(VERSION 3.20)

project(local_cpp_video_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(local_cpp_video_pipeline
    src/main.cpp
)

if (MSVC)
    target_compile_options(local_cpp_video_pipeline PRIVATE /W4 /permissive-)
else()
    target_compile_options(local_cpp_video_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()

