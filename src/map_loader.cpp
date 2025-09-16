/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// @Todo: This loader is mangled with metaprogramming module, which isn't neat.
//        Adds crazy blackbox to the code. This must be resolved.

#include "map_loader_lexer.cpp"

struct Parser 
{
    Lexer *lexer;
    u32 token_cursor = 0;

    Game_State *game_state;
};

void init(Lexer *lexer, Utf8 input, Parser *parser, Game_State *game_state) 
{
    lexer->input = input;
    parser->lexer = lexer;
    parser->game_state = game_state;
}

Token *peek_token(Parser *parser, int ahead = 0) 
{
    Assert(parser->token_cursor + ahead < parser->lexer->token_count);
    return parser->lexer->tokens + parser->token_cursor + ahead;
}

void eat_token(Parser *parser) 
{
    ++parser->token_cursor;
}

s32 parse_integer(Parser *parser) 
{
    Token *token = peek_token(parser);
    Assert(token && token->type == TOKEN_INTEGER);
    eat_token(parser);
    return token->integer_value;
}

f32 parse_float(Parser *parser) 
{
    f32 result;

    Token *token = peek_token(parser);
    Assert(token && token->type == TOKEN_FLOAT);

    if (peek_token(parser, 1)->type == TOKEN_COLON) {
        eat_token(parser); // FLOAT
        eat_token(parser); // :
        token = peek_token(parser);
        Assert(token->type == TOKEN_RAW);
        *(u32 *)&result = (u32)token->raw_value;
    } else {
        result = token->float_value;
    }

    eat_token(parser);
    return result;
}

#include "generated/entity_deserialization.h"

#define PARSE_ENTITY(TYPE)\
    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, #TYPE)) {\
        parse_##TYPE(parser);\
        break;\
    }

void parse(Parser *parser) 
{
    Lexer *lexer = parser->lexer;
    while (1) {
        Token *token = peek_token(parser);
        switch (token->type) {
            case TOKEN_IDENT:
            {
#include "generated/entity_parse.inl" // @Note: Metaprogramming: Ugly, but kinda works.
                INVALID_CODE_PATH;
            } break;

            case TOKEN_END_OF_INPUT:
            return;

            INVALID_DEFAULT_CASE;
        }
    }
}

void load_map(Utf8 file_path, Game_State *game_state) 
{
    Temporary_Arena scratch = scratch_begin();

#if __DEVELOPER
    u64 pc_begin = os.perf_counter();
#endif

    Utf8 input = read_entire_file(scratch.arena, file_path);

    Lexer *lexer = new Lexer(); // @Temporary
    Parser parser = {};
    init(lexer, input, &parser, game_state);
    lexical_analysis(lexer);
    parse(&parser);

    delete lexer;

#if __DEVELOPER
    u64 pc_end = os.perf_counter();
    f32 elapsed_ms = 1000.0f * (pc_end - pc_begin) * os.perf_counter_freq_inv;
    printf("Loaded map '%s' in %.2fms.\n", file_path.str, elapsed_ms);
#endif

    scratch_end(scratch);
}
