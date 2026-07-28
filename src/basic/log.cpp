// Copyright Seong Woo Lee. All Rights Reserved.


void log(String fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String str = tprint((char *)fmt.str, args);
    va_end(args);
    printf("%s\n", str.str);
}
