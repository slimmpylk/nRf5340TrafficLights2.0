#include "TimeParser.h"
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <iostream> // For debug printing

// Error codes
#define TIME_LEN_ERROR -1
#define INVALID_FORMAT_ERROR -2
#define INVALID_TIME_ERROR -3
#define NULL_POINTER_ERROR -4
#define ZERO_TIME_ERROR -5

int time_parse(const char *time) {
    // Check if the input is null
    if (time == nullptr) {
        std::cerr << "Error: Null pointer provided as input." << std::endl;
        return NULL_POINTER_ERROR;  // Null pointer error
    }

    // Check length - should be 6 digits for HHMMSS
    if (strlen(time) != 6) {
        std::cerr << "Error: Time string length is not 6. Length provided: " << strlen(time) << std::endl;
        return TIME_LEN_ERROR;  // Length should be exactly 6
    }

    // Ensure all characters are numeric
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            std::cerr << "Error: Invalid character found at index " << i << ": " << time[i] << std::endl;
            return INVALID_FORMAT_ERROR;  // Return error code for invalid format
        }
    }

    // Extract hours, minutes, and seconds
    int hours = (time[0] - '0') * 10 + (time[1] - '0');
    int minutes = (time[2] - '0') * 10 + (time[3] - '0');
    int seconds = (time[4] - '0') * 10 + (time[5] - '0');

    // Validate extracted values
    if (hours < 0 || hours > 23) {
        std::cerr << "Error: Invalid hour value: " << hours << std::endl;
        return INVALID_TIME_ERROR;
    }
    if (minutes < 0 || minutes > 59) {
        std::cerr << "Error: Invalid minute value: " << minutes << std::endl;
        return INVALID_TIME_ERROR;
    }
    if (seconds < 0 || seconds > 59) {
        std::cerr << "Error: Invalid second value: " << seconds << std::endl;
        return INVALID_TIME_ERROR;
    }

    // Check if the total time is zero
    if (hours == 0 && minutes == 0 && seconds == 0) {
        std::cerr << "Error: Time value cannot be zero (00:00:00)." << std::endl;
        return ZERO_TIME_ERROR;  // Time cannot be zero
    }

    // Return total time in seconds
    return hours * 3600 + minutes * 60 + seconds;
}
