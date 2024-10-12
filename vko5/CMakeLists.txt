cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(vko5 C CXX)

target_sources(app PRIVATE src/main.c src/TimeParser.cpp src/TrafficParser.cpp)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Set the C++ standard if needed
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
