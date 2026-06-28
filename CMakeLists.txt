cmake_minimum_required(VERSION 3.15)

# Name your project
project(KV_Database LANGUAGES CXX)

# C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories so CMake knows where to look for header files (.h)
include_directories(src/RAM src/Server src/Storage)

# Build the executable
add_executable(KV_Database src/main.cpp src/RAM/HashMap.cpp src/Server/Server.cpp src/Storage/Persistence.cpp)

# Add socket lib
target_link_libraries(KV_Database ws2_32)
