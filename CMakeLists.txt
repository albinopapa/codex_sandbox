cmake_minimum_required(VERSION 3.16)
project(SoftwareRenderer LANGUAGES CXX)

# Only provide a default for single-config generators when no build type is set.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(software_renderer_demo src/main.cpp)
target_include_directories(software_renderer_demo PRIVATE src)

if(WIN32)
    target_compile_definitions(software_renderer_demo PRIVATE UNICODE _UNICODE)
endif()
