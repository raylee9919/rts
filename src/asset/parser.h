// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "basic/core.h"
#include "basic/arena.h"
#include "basic/string.h"
#include "math/math.h"

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

    vec2 parse_v2(Parser *p);

    vec3 parse_v3(Parser *p);

    vec4 parse_v4(Parser *p);

    m4x4 parse_m4x4(Parser *p);

    Quaternion parse_quaternion(Parser *p);

    String parse_string_by_line(Parser *p, Arena *arena);

    String parse_string_by_length(Parser *p, u8 length, Arena *arena);

    bool is_eof(Parser *p);
}
