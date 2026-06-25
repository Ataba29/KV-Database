cmake_minimum_required(VERSION 3.15)

# Name your project
project(KV_Database LANGUAGES CXX)

# C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories so CMake knows where to look for header files (.h)
include_directories(src/RAM)

# Build the executable
add_executable(KV_Database src/main.cpp src/RAM/HashMap.cpp)