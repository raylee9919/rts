// Copyright Seong Woo Lee. All Rights Reserved.


#include "./text_file_handler.h"
#include "basic/log.h"

void Text_File_Handler::start(String in_file_data) 
{
    file_data = in_file_data;

    if ( parse_version_number ) 
    {
        auto [line, found] = ::consume_next_line(&file_data);
        line_number += 1;

        if ( !found ) 
        {
            log_error(S("Unable to find a version number at the top of file."));
            return;
        }

        R_ASSERT( line.len > 0 );
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

Pair<String, b32> Text_File_Handler::consume_next_line()
{
    while (true)
    {
        auto [line, found] = ::consume_next_line(&file_data);
        if (!found) return { {}, false };

        line_number += 1;

        line = eat_spaces(line);

        if (!line)
        {
            if ( auto_skip_blank_lines ) continue;
            return { {}, true };
        }

        if ( strip_comments_from_ends_of_lines )
        {
            auto [found, left, right] = split_from_left(line, comment_character);
            if (found)
            {
                line = left;
                if (line.len == 0) continue;
            }
        }
        else
        {
            if ( line[0] == comment_character ) continue;
        }

        line = eat_trailing_spaces(line);
        R_ASSERT( line.len > 0 );

        return { line, found };
    }
}

Pair<String, b32> consume_next_line(String *pstr) 
{
    String s = *pstr;
    auto [found, left, right] = split_from_left(s, 10); // LF

    if (!found) // You are on the last line
    {
        *pstr = S("");

        return { s, s.len > 0 };
    }

    advance(pstr, left.len + 1);

    if (left)
    {
        if ( left[left.len - 1] == 13 ) // If 'CR' is at the end, remove it by decrementing the string's length.
        {
            left.len -= 1;
        }
    }

    return { left, true };
}

Pair<String, String> break_by_spaces(String line) 
{
    String left = line;
    String right = eat_until_space(line);
    left.len -= right.len;

    right = eat_spaces(right);

    return { left, right };
}
