// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_LOG_H
#define RTS_LOG_H


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
    _log_internal(level, fmt, S(__FILE__), __LINE__,  ##__VA_ARGS__)
internal void _log_internal(Log_Level level, String fmt, String file, int line, ...);


#endif // RTS_LOG_H
