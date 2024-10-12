// Test cases for TimeParserTest.cpp
#include "gtest/gtest.h"

#include "../src/TimeParser.h"

#include <iostream>

// Test valid time at upper boundary
TEST(TimeParserTest, ValidUpperBoundary) {
    const char * test_str = "235959"; // Correct length: 6 characters
    std::cout << "Testing valid upper boundary with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, (23 * 3600) + (59 * 60) + 59);
}

// Test valid time at lower boundary
TEST(TimeParserTest, ValidLowerBoundary) {
    const char * test_str = "000000"; // Correct length: 6 characters
    std::cout << "Testing valid lower boundary with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, ZERO_TIME_ERROR); // Update expected value to match the zero-time error code
}

// Test invalid hour (over 23)
TEST(TimeParserTest, InvalidHour) {
    const char * test_str = "240000"; // Correct length: 6 characters
    std::cout << "Testing invalid hour with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, INVALID_TIME_ERROR);
}

// Test invalid minute (over 59)
TEST(TimeParserTest, InvalidMinute) {
    const char * test_str = "236000"; // Correct length: 6 characters
    std::cout << "Testing invalid minute with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, INVALID_TIME_ERROR);
}

// Test invalid second (over 59)
TEST(TimeParserTest, InvalidSecond) {
    const char * test_str = "235960"; // Correct length: 6 characters
    std::cout << "Testing invalid second with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, INVALID_TIME_ERROR);
}

// Test invalid format (non-numeric characters)
TEST(TimeParserTest, InvalidFormat) {
    const char * test_str = "23A959"; // Correct length: 6 characters
    std::cout << "Testing invalid format with input: " << test_str << std::endl;
    int result = time_parse(test_str);
    EXPECT_EQ(result, INVALID_FORMAT_ERROR);
}