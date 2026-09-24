// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_LOG_H
#define RTS_LOG_H

#include "basic/core.h"
#include "basic/string.h"


#define ANSI_COLOR_RESET            "\033[0m"
#define ANSI_COLOR_RED              "\033[31m"
#define ANSI_COLOR_GREEN            "\033[32m"
#define ANSI_COLOR_YELLOW           "\033[33m"
#define ANSI_COLOR_BLUE             "\033[34m"
#define ANSI_COLOR_PURPLE           "\033[35m"
#define ANSI_COLOR_CYAN             "\033[36m"
#define ANSI_COLOR_WHITE            "\033[37m"
#define ANSI_COLOR_BRIGHT_BLACK     "\033[90m"
#define ANSI_COLOR_BRIGHT_RED       "\033[91m"
#define ANSI_COLOR_BRIGHT_GREEN     "\033[92m"
#define ANSI_COLOR_BRIGHT_YELLOW    "\033[93m"
#define ANSI_COLOR_BRIGHT_BLUE      "\033[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA   "\033[95m"
#define ANSI_COLOR_BRIGHT_CYAN      "\033[96m"
#define ANSI_COLOR_BRIGHT_WHITE     "\033[97m"

enum Log_Level : u8 {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
};

#define log(level, fmt, ...) \
    _log_internal(level, tprint(fmt, ##__VA_ARGS__), S(__FILE__), __LINE__)
void _log_internal(Log_Level level, String msg, String file, int line, bool just_print = false);

#define print(fmt, ...) _log_internal(LOG_INFO, tprint(fmt, ##__VA_ARGS__), S(__FILE__), __LINE__, true)

#define log_trace(fmt, ...)     log(LOG_TRACE,   fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...)     log(LOG_DEBUG,   fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)      log(LOG_INFO,    fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...)   log(LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)     log(LOG_ERROR,   fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...)     log(LOG_FATAL,   fmt, ##__VA_ARGS__)


#endif // RTS_LOG_H
