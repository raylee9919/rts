// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_TEXT_FILE_HANDLER_H
#define RTS_TEXT_FILE_HANDLER_H

#include "basic/core.h"
#include "basic/string.h"

struct Text_File_Handler {
    u8 comment_character = '#';

    b32 parse_version_number              = true;
    b32 auto_skip_blank_lines             = true;
    b32 strip_comments_from_ends_of_lines = true;

    s64 version = -1;

    s64 line_number = 0;

    String file_data;


    void start(String in_file_data);
    Pair<String, b32> consume_next_line();
};

Pair<String, b32> consume_next_line(String *sp);
Pair<String, String> break_by_spaces(String line);

#endif // RTS_TEXT_FILE_HANDLER_H
