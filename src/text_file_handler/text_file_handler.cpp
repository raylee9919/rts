// Copyright Seong Woo Lee. All Rights Reserved.


#include "./text_file_handler.h"
#include "basic/log.h"

void Text_File_Handler::start(String in_file_data) {
    file_data = in_file_data;

    if ( do_version_number ) 
    {
        auto [line, found] = consume_next_line(&file_data);
        line_number += 1;

        if ( !found ) 
        {
            log_error(S("Unable to find a version number at the top of file."));
            return;
        }

        Assert( line.len > 0 );
        if ( line.str[0] != '[' ) {
            log_error(S("Expected '[' at the top of file."));
            return;
        }

        advance(&line, 1);

        auto [ver, success, remainder] = int_from_string(line);
        if ( !success ) 
        {
            log_error(S("Unable to parse the version number at the top of file."));
            return;
        }

        if ( (!remainder.len) || (remainder.str[0] != ']') ) 
        {
            log_error(S("Expected ']' after version number."));
            return;
        }

        version = ver;
    }
}

Pair<String, b32> consume_next_line(String *sp) {
    // To find the end of the line, we look for a linefeed character. 
    // We will trim a carriage return off the end if there is one there also. 
    // Thus this works on both 'DOS' and 'Unix'-style files.

    String s = *sp;
    String line, remainder;
    b32 found = split_from_left(s, 10, &line, &remainder); // LF

    if ( !found ) {
        // This is the last line; there may not have been a LF after that, 
        // but we still want to handle that data, so we return true if there was 
        // a non-zero amount of stuff there.

        sp->str = 0;

        return { s, s.len > 0};
    }

    // Chop the characters we are going to return from 'sp', 
    // which holds the remaining file data.
    advance(sp, line.len + 1);

    if ( line.str[line.len] ) {
        if ( line.str[line.len - 1] == 13 )  line.len -= 1; // If there's a CR at the end, remove it by decrementing the string's length.
    }

    return { line, true };
}

Pair<String, String> break_by_spaces(String line) {
    String left = line;
    String right = eat_until_space(line);
    left.len -= right.len;

    right = eat_spaces_from_left(right);

    return { left, right };
}
