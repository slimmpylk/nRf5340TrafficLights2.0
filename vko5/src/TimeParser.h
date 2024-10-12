#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#define TIME_LEN_ERROR -1
#define INVALID_FORMAT_ERROR -2
#define INVALID_TIME_ERROR -3
#define NULL_POINTER_ERROR -4
#define ZERO_TIME_ERROR -5


#ifdef __cplusplus
extern "C" {
#endif

int time_parse(const char* time_str);

#ifdef __cplusplus
}
#endif

#endif // TIMEPARSER_H
