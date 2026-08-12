// Copyright Seong Woo Lee. All Rights Reserved.


void _log_internal(Log_Level level, String fmt, String file, int line, ...) 
{
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

    {
        va_list args;
        va_start(args, line);
        String str = tprint((char *)fmt.str, args);
        va_end(args);

        printf("%s", str.str);
    }

    printf("\n");
}
