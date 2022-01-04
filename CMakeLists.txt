cmake_minimum_required(VERSION 3.17)
project(pmo)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS)
set(CMAKE_CXX_FLAGS_DEBUG)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -pthread")

include_directories(include)
add_executable(${PROJECT_NAME} main.cpp)
