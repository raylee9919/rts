/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define MAX_TOKEN_LENGTH 256

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

    Buffer input;
    s32 input_cursor = 0;

    Token tokens[100000]; // @Temporary
    u32 token_count = 0;
};

inline bool starts_identifier(int c) {
    if (is_alpha(c)) return true;
    if (c == '_') return true;
    return false;
}

inline bool continues_identifier(int c) {
    if (is_alnum(c)) return true;
    if (c == '_') return true;
    return false;
}

inline bool starts_number(int c) {
    if (is_digit(c) || (c == '-')) return true;
    return false;
}

void eat_character(Lexer *lexer) {
    if (lexer->input.data[lexer->input_cursor] == '\n') {
        // DoThisOnNewLine
        lexer->current_line_number++;
        lexer->total_lines_processed++;
        lexer->current_character_index = 0;
    }

    lexer->input_cursor++;
    lexer->current_character_index++;
}

int peek_next_character(Lexer *lexer, int ahead = 0) {
    Assert(lexer->input.data != 0);

    if (lexer->input_cursor + ahead >= lexer->input.count) {
        return -1;
    }

    int result = lexer->input.data[lexer->input_cursor + ahead];
    return result;
}

void push_token(Lexer *lexer, Token token) {
    Assert(lexer->token_count < arraycount(lexer->tokens));
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

void lexical_analysis(Lexer *lexer) {
    while (lexer->input_cursor <= lexer->input.count) {
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
