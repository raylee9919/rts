// Copyright Seong Woo Lee. All Rights Reserved.


void _log(String fmt, String file, int line, ...) {
    {
        char *file_line_fmt = "[%S: %d]  ";
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
