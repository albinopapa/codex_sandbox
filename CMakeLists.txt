cmake_minimum_required(VERSION 3.16)
project(SoftwareRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(software_renderer_demo src/main.cpp)
target_include_directories(software_renderer_demo PRIVATE src)
