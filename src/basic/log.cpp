// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/log.h"
#include "basic/string.h"

#include <stdio.h>


void _log_internal(Log_Level level, String msg, String file, int line, bool just_print)
{
    if (!just_print) {
        {
            String str_level = {};
            if      (level == LOG_TRACE)   str_level = tprint(S("[%s%+5s%s] "),        ANSI_COLOR_CYAN,   "TRACE", ANSI_COLOR_RESET);
            else if (level == LOG_DEBUG)   str_level = tprint(S("[%s%+5s%s] "),       ANSI_COLOR_GREEN,   "DEBUG", ANSI_COLOR_RESET);
            else if (level == LOG_INFO)    str_level = tprint(S("[%s%+5s%s] "), ANSI_COLOR_BRIGHT_CYAN,    "INFO", ANSI_COLOR_RESET);
            else if (level == LOG_WARNING) str_level = tprint(S("[%s%+5s%s] "),      ANSI_COLOR_YELLOW,    "WARN", ANSI_COLOR_RESET);
            else if (level == LOG_ERROR)   str_level = tprint(S("[%s%+5s%s] "),  ANSI_COLOR_BRIGHT_RED,   "ERROR", ANSI_COLOR_RESET);
            else if (level == LOG_FATAL)   str_level = tprint(S("[%s%+5s%s] "),         ANSI_COLOR_RED,   "FATAL", ANSI_COLOR_RESET);

            printf((char *)str_level.str);
        }

        if (0)
        {
            String file_line_fmt = S("%S(%d)  ");
            String file_line = tprint(file_line_fmt, file, line);
            printf("%s", file_line.str);
        }
    }

    printf("%.*s", (int)msg.len, msg.str);

    printf("\n");
}
