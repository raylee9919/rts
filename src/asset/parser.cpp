// Copyright Seong Woo Lee. All Rights Reserved.

#include "asset/parser.h"
#include "basic/arena.h"
#include "basic/string.h"

namespace Asset
{
    void init(Parser* p, void *ptr, u64 size)
    {
        p->cursor = (u8*)ptr;
        p->end = p->cursor + size;
    }

    void eat_whitespace(Parser* p) 
    {
        while (p->cursor < p->end) {
            u8 c = *p->cursor;
            if (!is_space(c)) break;
            p->cursor++;
        }
    }

    u8 peek(Parser* p) 
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        return *p->cursor;
    }

    u8 eat(Parser* p) 
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        u8 c = *p->cursor++;
        return c;
    }

    u32 parse_u32(Parser *p)
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        eat_whitespace(p);

        u32 result = 0;

        while (p->cursor < p->end) {
            u8 c = peek(p);
            if (is_digit(c)) {
                u32 num = c - '0';
                result *= 10;
                result += num;
                p->cursor++;
            } else {
                break;
            }
        }

        return result;
    }

    Version parse_version(Parser *p)
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        eat_whitespace(p);

        Version ver = {};

        if (peek(p) == 'v') {
            eat(p);
            ver.major = (u8)parse_u32(p);
            R_ASSERT(peek(p) == '.');
            eat(p);
            ver.minor = (u8)parse_u32(p);
            R_ASSERT(peek(p) == '.');
            eat(p);
            ver.patch = (u8)parse_u32(p);
        }

        return ver;
    }

    s32 parse_s32(Parser *p) 
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        eat_whitespace(p);

        bool sign = false;
        u8 sign_char = peek(p);
        if (sign_char == '+') {
            eat(p);
        } else if (sign_char == '-') {
            sign = true;
            eat(p);
        }

        u32 integer = parse_u32(p);
        R_ASSERT(integer <= 0x0fffffff);
        s32 result = (u32)integer;

        if (sign) {
            result = -result;
        }

        return result;
    }

    f32 parse_f32(Parser *p) 
    {
        R_ASSERT(p->cursor && p->cursor < p->end);
        eat_whitespace(p);

        bool sign = false;
        u8 sign_char = peek(p);
        if (sign_char == '+') {
            eat(p);
        } else if (sign_char == '-') {
            sign = true;
            eat(p);
        }

        s32 integer = 0;
        while (p->cursor < p->end) {
            u8 c = peek(p);
            if (is_digit(c)) {
                s32 num = c - '0';
                integer *= 10;
                integer += num;
                p->cursor++;
            } else {
                break;
            }
        }

        f32 fraction = 0.f;
        f32 weight = 0.1f;
        if (peek(p) == '.') {
            p->cursor++;

            while (p->cursor < p->end) {
                char c = peek(p);
                if (is_digit(c)) {
                    f32 num = (f32)(c - '0');
                    fraction += (num*weight);
                    weight *= 0.1f;
                    p->cursor++;
                } else {
                    break;
                }
            }
        }

        f32 result = (f32)integer + fraction;
        if (sign) {
            result = -result;
        }
        return result;
    }

    vec2 parse_v2(Parser *p) 
    {
        vec2 result;
        result.x = parse_f32(p);
        result.y = parse_f32(p);
        return result;
    }

    vec3 parse_v3(Parser *p) 
    {
        vec3 result;
        result.x = parse_f32(p);
        result.y = parse_f32(p);
        result.z = parse_f32(p);
        return result;
    }

    vec4 parse_v4(Parser *p) 
    {
        vec4 result;
        result.r = parse_f32(p);
        result.g = parse_f32(p);
        result.b = parse_f32(p);
        result.a = parse_f32(p);
        return result;
    }

    m4x4 parse_m4x4(Parser *p) 
    {
        m4x4 result;
        result.rows[0] = parse_v4(p);
        result.rows[1] = parse_v4(p);
        result.rows[2] = parse_v4(p);
        result.rows[3] = parse_v4(p);
        return result;
    }

    Quaternion parse_quaternion(Parser *p) 
    {
        Quaternion result;
        result.w = parse_f32(p);
        result.x = parse_f32(p);
        result.y = parse_f32(p);
        result.z = parse_f32(p);
        return result;
    }

    String parse_string_by_line(Parser *p, Arena *arena) 
    {
        String result = {};

        int len = 0;
        for (;;) {
            u8 c = peek(p);
            if (c == '\r' || c == '\n') {
                break;
            }

            len++;
            eat(p);
        }

        result.len = len;
        result.str = push_array(arena, u8, len + 1);
        memcpy(result.str, p->cursor - len, len);

        return result;
    }

    String parse_string_by_length(Parser *p, u8 length, Arena *arena) 
    {
        String result = {};

        R_ASSERT(p->cursor && p->cursor < p->end);
        eat_whitespace(p);

        result.len = length;
        result.str = push_array(arena, u8, length + 1);
        memcpy(result.str, p->cursor, length);

        p->cursor += length;

        return result;
    }

    bool is_eof(Parser *p)
    {
        eat_whitespace(p);
        return p->cursor == p->end;
    }
};
