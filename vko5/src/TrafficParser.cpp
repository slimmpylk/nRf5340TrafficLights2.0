#include "TrafficParser.h"

#include <cstring>

#include <cctype>

#include <cstdio>

int sequence_parse(const char * sequence, command_t * parsed_commands, int * command_count) {
    if (sequence == nullptr || strlen(sequence) == 0) {
        return SEQUENCE_FORMAT_ERROR; // Invalid format or empty sequence
    }

    * command_count = 0; // Initialize command count

    const char * ptr = sequence;
    char command;
    int duration;

    while ( * ptr != '\0') {
        // Skip any commas or spaces
        while ( * ptr == ',' || * ptr == ' ') {
            ++ptr;
        }

        // Check for end of string
        if ( * ptr == '\0') {
            break;
        }

        // Parse command
        if ( * ptr == 'R' || * ptr == 'Y' || * ptr == 'G') {
            command = * ptr;
            ++ptr;

            // Default duration
            duration = -1;

            // Check if duration is specified
            if ( * ptr == ',') {
                ++ptr;
                // Parse duration
                if (sscanf(ptr, "%d", & duration) != 1) {
                    return SEQUENCE_FORMAT_ERROR; // Invalid format
                }
                // Move pointer past the duration
                while (isdigit( * ptr)) {
                    ++ptr;
                }
            }

            // Add command to array
            parsed_commands[ * command_count].color = command;
            parsed_commands[ * command_count].duration = duration;
            ( * command_count) ++;

            // Check for array overflow
            if ( * command_count >= MAX_COMMANDS) {
                return SEQUENCE_FORMAT_ERROR; // Too many commands
            }
        } else {
            return UNKNOWN_COMMAND_ERROR; // Invalid command character
        }
    }

    return 0; // Successful parsing
}