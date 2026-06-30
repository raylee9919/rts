// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace Asset
{
    struct Parser
    {
        u8* cursor;
        u8* end;
    };

    void init(Parser* p, void *ptr, u64 size);

    void eat_whitespace(Parser *p);

    u8 peek(Parser *p);

    u8 eat(Parser *p);

    u32 parse_u32(Parser *p);

    s32 parse_s32(Parser *p);

    f32 parse_f32(Parser *p);

    v2 parse_v2(Parser *p);

    v3 parse_v3(Parser *p);

    v4 parse_v4(Parser *p);

    m4x4 parse_m4x4(Parser *p);

    Quaternion parse_quaternion(Parser *p);

    String parse_string_by_line(Parser *p, Arena *arena);

    String parse_string_by_length(Parser *p, u8 length, Arena *arena);

    bool is_eof(Parser *p);
}
