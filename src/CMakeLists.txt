cmake_minimum_required(VERSION 3.15)

# Name your project
project(KV_Database LANGUAGES CXX)

# C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MyExecutable main.cpp "HashMap/Node.h")