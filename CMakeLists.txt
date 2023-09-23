cmake_minimum_required(VERSION 3.20)
project(qt6-coro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED
  Test
  Network
)

enable_testing()

add_subdirectory(coro)
add_subdirectory(examples)
add_subdirectory(tests)
