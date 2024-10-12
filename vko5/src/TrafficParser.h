#ifndef TRAFFIC_PARSER_H
#define TRAFFIC_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

// Error codes for Traffic Parser
#define SEQUENCE_FORMAT_ERROR -6
#define UNKNOWN_COMMAND_ERROR -7

#define MAX_COMMANDS 10  // Define the maximum number of commands

typedef struct {
    char color;
    int duration;
} command_t;

// Function to parse the traffic light sequence
int sequence_parse(const char *sequence, command_t *parsed_commands, int *command_count);

#ifdef __cplusplus
}
#endif

#endif // TRAFFIC_PARSER_H
