#ifndef RTS_MAP_LOADER_H
#define RTS_MAP_LODAER_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#define MAX_TOKEN_LENGTH 256
#define PARSE_ENTITY(TYPE)\
    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, #TYPE)) {\
        parse_##TYPE(parser);\
        break;\
    }


enum Token_Type {
    TOKEN_END_OF_INPUT = -1,
    TOKEN_ERROR = 0,

    TOKEN_IDENT   = 256,
    TOKEN_INTEGER = 257,
    TOKEN_FLOAT   = 258,
    TOKEN_RAW     = 259,

    // ASCII
    TOKEN_MINUS = '-',
    TOKEN_PERIOD = '.',
    TOKEN_COLON = ':',
    TOKEN_SEMICOLON = ';',
};

struct Token {
    Token_Type type = TOKEN_ERROR;

    union {
        u64 integer_value;
        f64 float_value;
        umm raw_value;
    };

    char scratch_buffer[MAX_TOKEN_LENGTH];
    u32 scratch_buffer_length;
};

struct Lexer {
    s32 current_line_number = 1;
    s32 current_character_index = 1;
    s32 total_lines_processed = 0;

    Utf8 input;
    s32 input_cursor = 0;

    Token tokens[100000]; // @Temporary
    u32 token_count = 0;
};

struct Parser 
{
    Lexer *lexer;
    u32 token_cursor = 0;

    Game_State *game_state;
};


internal b32 starts_identifier(int c);
internal b32 continues_identifier(int c);
internal b32 starts_number(int c);
internal void eat_character(Lexer *lexer);
internal int peek_next_character(Lexer *lexer, int ahead);
void push_token(Lexer *lexer, Token token);
void push_eof(Lexer *lexer);
void push_single_token(Lexer *lexer);
void push_identifier(Lexer *lexer);
Token lex_decimal(Lexer *lexer);
Token lex_hexadecimal(Lexer *lexer);
void push_number(Lexer *lexer);
void lexical_analysis(Lexer *lexer);



void init(Lexer *lexer, Utf8 input, Parser *parser, Game_State *game_state);
Token *peek_token(Parser *parser, int ahead);
void eat_token(Parser *parser);
s32 parse_integer(Parser *parser);
f32 parse_float(Parser *parser);
void parse(Parser *parser);
void load_map(Utf8 file_path, Game_State *game_state);


#endif // RTS_MAP_LOADER_H
