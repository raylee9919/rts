/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// @Todo: This loader is mangled with metaprogramming module, which isn't neat.
//        Adds crazy blackbox to the code. This must be resolved.

internal b32
starts_identifier(int c) 
{
    if (is_alpha(c)) return true;
    if (c == '_') return true;
    return false;
}

internal b32
continues_identifier(int c) 
{
    if (is_alnum(c)) return true;
    if (c == '_') return true;
    return false;
}

internal b32
starts_number(int c) 
{
    if (is_digit(c) || (c == '-')) return true;
    return false;
}

internal void
eat_character(Lexer *lexer) 
{
    if (lexer->input.str[lexer->input_cursor] == '\n') 
    {
        // DoThisOnNewLine
        lexer->current_line_number++;
        lexer->total_lines_processed++;
        lexer->current_character_index = 0;
    }

    lexer->input_cursor++;
    lexer->current_character_index++;
}

internal int
peek_next_character(Lexer *lexer, int ahead = 0) 
{
    Assert(lexer->input.str != 0);

    if (lexer->input_cursor + ahead >= lexer->input.len) 
    { return -1; }

    int result = lexer->input.str[lexer->input_cursor + ahead];
    return result;
}

void push_token(Lexer *lexer, Token token) {
    Assert(lexer->token_count < array_count(lexer->tokens));
    lexer->tokens[lexer->token_count++] = token;
}

void push_eof(Lexer *lexer) {
    Token token = {};
    token.type = TOKEN_END_OF_INPUT;
    push_token(lexer, token);
}

void push_single_token(Lexer *lexer) {
    int c = peek_next_character(lexer);
    Token token = {};
    token.type = (Token_Type)c;
    push_token(lexer, token);
}

void push_identifier(Lexer *lexer) {
    Token token = {};
    token.type = TOKEN_IDENT;

    while (1) {
        int c = peek_next_character(lexer);
        if (continues_identifier(c)) {
            eat_character(lexer);
            token.scratch_buffer[token.scratch_buffer_length++] = c;
        } else {
            break;
        }
    }

    push_token(lexer, token);
}

Token lex_decimal(Lexer *lexer) {
    Token token = {};

    int c = peek_next_character(lexer);
    b32 sign = false;
    if (c == '-') {
        sign = true;
        eat_character(lexer);
    }

    b32 is_float = false;
    f32 value = 0;
    s32 x = 1;
    while (1) {
        c = peek_next_character(lexer);

        if (is_digit(c)) {
            f32 d = atoi(c);
            if (!is_float) {
                value = value * 10 + d;
            } else {
                value += d * pow(0.1f, x++);
            }
            eat_character(lexer);
        } else if (c == '.') {
            is_float = true;
            eat_character(lexer);
        } else {
            break;
        }
    }

    if (sign) value = -value;

    if (is_float) {
        token.type = TOKEN_FLOAT;
        token.float_value = value;
    } else {
        token.type = TOKEN_INTEGER;
        token.integer_value = s32(value + 0.5f);
    }

    return token;
}

Token lex_hexadecimal(Lexer *lexer) {
    Token token = {};

    eat_character(lexer); // 0
    eat_character(lexer); // x

    umm value = 0;
    while (1) {
        int c = peek_next_character(lexer);
        if (is_hexdigit(c)) {
            umm h = atoh(c);
            value = (value << 4) + h;
            eat_character(lexer);
        } else {
            break;
        }
    }

    token.type = TOKEN_RAW;
    token.raw_value = value;

    return token;
}

void push_number(Lexer *lexer) {
    Token token = {};

    if (peek_next_character(lexer) == '0') {
        if (peek_next_character(lexer, 1) == 'x') {
            token = lex_hexadecimal(lexer);
        } else if (peek_next_character(lexer, 1) == '.') {
            token = lex_decimal(lexer);
        } else {
            token = lex_decimal(lexer);
        }
    } else {
        token = lex_decimal(lexer);
    }

    push_token(lexer, token);
}

void lexical_analysis(Lexer *lexer) 
{
    while (lexer->input_cursor <= lexer->input.len) 
    {
        int c = peek_next_character(lexer);
        switch (c) {
            case -1:
            push_eof(lexer);
            return;

            case ':': 
            case ';': 
            case '.':
            {
                push_single_token(lexer);
                eat_character(lexer);
            } break;

            default: 
            {
                if (starts_identifier(c)) {
                    push_identifier(lexer);
                } else if (starts_number(c)) {
                    push_number(lexer);
                } else if (is_whitespace(c)) {
                    eat_character(lexer);
                } else {
                    INVALID_CODE_PATH;
                }
            } break;
        }
    }
}


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
