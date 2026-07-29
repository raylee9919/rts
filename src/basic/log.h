// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_LOG_H
#define RTS_LOG_H


#define log(fmt, ...) _log(fmt, S(__FILE__), __LINE__,  ##__VA_ARGS__)
internal void _log(String fmt, String file, int line, ...);


#endif // RTS_LOG_H
