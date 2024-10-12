# CMakeLists.txt for GoogleTest
cmake_minimum_required(VERSION 3.20)

# Define the test project and set the C++ standard
project(Vko5Tests CXX)

# Specify the C++ standard version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define the directories for GoogleTest and the source files
set(GTEST_DIR ${CMAKE_SOURCE_DIR}/../googletest)
set(VKO5_DIR ${CMAKE_SOURCE_DIR}/../vko5/src)

# Include GoogleTest if the directory exists
if (EXISTS ${GTEST_DIR})
    add_subdirectory(${GTEST_DIR} googletest-build)
else()
    message(FATAL_ERROR "GoogleTest directory not found at ${GTEST_DIR}")
endif()

# Add executable for TimeParser tests
add_executable(TimeParserTest
    TimeParserTest.cpp
    ${VKO5_DIR}/TimeParser.cpp
)
# Add include directory for headers
include_directories(${CMAKE_SOURCE_DIR}/../vko5/src)

# Link GoogleTest libraries for TimeParserTest
target_link_libraries(TimeParserTest gtest gtest_main)

# Add executable for TrafficParser tests
add_executable(TrafficParserTest
    TrafficParserTest.cpp
    ${VKO5_DIR}/TrafficParser.cpp
)

# Link GoogleTest libraries for TrafficParserTest
target_link_libraries(TrafficParserTest gtest gtest_main)

# Add common warnings
if (MSVC)
    target_compile_options(TimeParserTest PRIVATE /W4)
    target_compile_options(TrafficParserTest PRIVATE /W4)
else()
    target_compile_options(TimeParserTest PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(TrafficParserTest PRIVATE -Wall -Wextra -pedantic)
endif()

# Enable GoogleTest
enable_testing()
include(GoogleTest)

# Register the tests with CTest
gtest_discover_tests(TimeParserTest)
gtest_discover_tests(TrafficParserTest)
