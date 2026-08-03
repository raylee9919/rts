// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_LOG_H
#define RTS_LOG_H


#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_BLUE    "\033[34m"
#define ANSI_COLOR_PINK    "\033[35m"
#define ANSI_COLOR_TEAL    "\033[36m"


#define log(fmt, ...) _log(fmt, S(__FILE__), __LINE__,  ##__VA_ARGS__)
internal void _log(String fmt, String file, int line, ...);


#endif // RTS_LOG_H
