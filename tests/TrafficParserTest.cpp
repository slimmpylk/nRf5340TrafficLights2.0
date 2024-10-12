#include "gtest/gtest.h"

#include "../vko5/src/TrafficParser.h"

// Test valid sequence in character-only format
TEST(TrafficParserTest, ValidCharacterOnlySequence) {
    const char * test_str = "RYGRYG";
    command_t parsed_commands[MAX_COMMANDS];
    int command_count;
    int result = sequence_parse(test_str, parsed_commands, & command_count);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(command_count, 6);
    EXPECT_EQ(parsed_commands[0].color, 'R');
    EXPECT_EQ(parsed_commands[1].color, 'Y');
    EXPECT_EQ(parsed_commands[2].color, 'G');
}

// Test valid sequence with duration format
TEST(TrafficParserTest, ValidDurationSequence) {
    const char * test_str = "R,1000,Y,500,G,1000";
    command_t parsed_commands[MAX_COMMANDS];
    int command_count;
    int result = sequence_parse(test_str, parsed_commands, & command_count);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(command_count, 3);
    EXPECT_EQ(parsed_commands[0].color, 'R');
    EXPECT_EQ(parsed_commands[0].duration, 1000);
    EXPECT_EQ(parsed_commands[1].color, 'Y');
    EXPECT_EQ(parsed_commands[1].duration, 500);
}

// Test invalid sequence with unsupported characters
TEST(TrafficParserTest, InvalidCharacterSequence) {
    const char * test_str = "RXGY";
    command_t parsed_commands[MAX_COMMANDS];
    int command_count;
    int result = sequence_parse(test_str, parsed_commands, & command_count);
    EXPECT_EQ(result, UNKNOWN_COMMAND_ERROR);
}

// Test empty sequence
TEST(TrafficParserTest, EmptySequence) {
    const char * test_str = "";
    command_t parsed_commands[MAX_COMMANDS];
    int command_count;
    int result = sequence_parse(test_str, parsed_commands, & command_count);
    EXPECT_EQ(result, SEQUENCE_FORMAT_ERROR);
}