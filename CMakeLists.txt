cmake_minimum_required(VERSION 3.15)

project(KV_Database LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++17 Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Turn our core logic into a LIBRARY so both the App and the Tests can use it
add_library(KV_Core
    src/RAM/HashMap.cpp
    src/Server/Server.cpp
    src/Storage/Persistence.cpp
    src/Worker/ThreadPool.cpp
    src/Worker/SnapshotScheduler.cpp
)

target_include_directories(KV_Core PUBLIC 
    src/RAM 
    src/Server 
    src/Storage 
    src/Worker
)

# Conditional library management depending on Target Environment OS
if(WIN32)
    target_link_libraries(KV_Core PRIVATE ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(KV_Core PRIVATE Threads::Threads)
endif()

# Build the main executable
add_executable(KV_Database src/main.cpp)
target_link_libraries(KV_Database PRIVATE KV_Core)

# =================================================================
# Testing Setup
# =================================================================

enable_testing()

# GoogleTest setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add the executable for our tests
add_executable(run_tests 
    src/Tests/test_hashmap.cpp
    src/Tests/test_threadpool.cpp
)

# Link the tests to GoogleTest and KV_Core
target_link_libraries(run_tests PRIVATE gtest_main KV_Core)

# Discover tests automatically
include(GoogleTest)
gtest_discover_tests(run_tests)