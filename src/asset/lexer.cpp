// Copyright Seong Woo Lee. All Rights Reserved.

#include "./lexer.h"
#include "shared.h"


// Peek next character in the input stream.
// Returns -1 if it's the end of the input.
static int
peek_char(Asset_Lexer *lexer, int index = 0)
{
  if (lexer->input_cursor + index >= lexer->input.len)
    return -1;
  return lexer->input.str[lexer->input_cursor + index];
}

// Eats one character in the input stream.
// Returns -1 if there was nothing left to eat.
static b32
eat_char(Asset_Lexer *lexer)
{
  int c = peek_char(lexer);
  if (c == -1) return false;
  if (c == '\n')
  {
    lexer->line += 1;
  }
  lexer->input_cursor += 1;
  return true;
}

static void
unwind_char(Asset_Lexer *lexer)
{
  R_ASSERT(lexer->input_cursor > 0);
  int c = lexer->input.str[lexer->input_cursor - 1];
  if (c == '\n')
  {
    lexer->line -= 1;
  }
  lexer->input_cursor -= 1;
}

static b32
_is_whitespace(int c)
{
  if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return true;
  return false;
}

static b32
_is_digit(int c)
{
  if (c >= '0' && c <= '9') return true;
  return false;
}

static b32
_is_alpha(int c)
{
  if (c >= 'A' && c <= 'Z') return true;
  if (c >= 'a' && c <= 'z') return true;
  return false;
}

static b32
_is_alnum(int c)
{
  if (is_digit(c) || is_alpha(c)) return true;
  return false;
}

static b32
_starts_identifier(int c)
{
  if (_is_alpha(c) || c == '_') return true;
  return false;
}

static b32
_continues_identifier(int c)
{
  if (_is_alnum(c) || c == '_') return true;
  return false;
}

static b32
_continues_single_line_comment(Asset_Lexer *lexer)
{
  int c = peek_char(lexer);
  if (lexer->input_cursor >= lexer->input.len) return false;
  if (c == '\n' || c == '\r') return false;
  return true;
}

static Asset_Token *
push_token(Asset_Lexer *lexer, Asset_Token_Type type)
{
  append(&lexer->tokens, Asset_Token{});
  Asset_Token *t = &lexer->tokens.data[lexer->tokens.count-1];
  t->type = type;
  return t;
}

static Asset_Token *
push_hex_float(Asset_Lexer *lexer)
{
  u32 num_digits  = 0;
  u64 accumulated = 0;

  for (;;)
  {
    u32 x;

    int c = peek_char(lexer);
    if      (c >= 'a' && c <= 'z')  x = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z')  x = c - 'A' + 10;
    else if (c >= '0' && c <= '9')  x = c - '0';
    else break;

    eat_char(lexer);

    accumulated *= 16;
    accumulated += x;

    num_digits += 1;
  }

  if (num_digits == 0)
  {
    R_ASSERT(0); // @Todo: Error-handling
  }
  else if ((num_digits != 4) && (num_digits != 8) && (num_digits != 16))
  {
    R_ASSERT(0); // @Todo: Error-handling
  }


  Asset_Token *t = push_token(lexer, ASSET_TOKEN_NUMBER);
  t->flags = ASSET_TOKEN_FLAG_FLOAT;

  union {
    f32 _f32;
    f64 _f64;
    u32 _u32;
    u64 _u64;
  } tmp;

  if (num_digits == 16)
  {
    tmp._u64 = accumulated;
    t->float64_value = tmp._f64;
    t->flags |= ASSET_TOKEN_FLAG_REQUIRES_FLOAT64;
  }
  else if (num_digits == 8)
  {
    tmp._u32 = (u32)accumulated;
    t->float64_value = tmp._f32; // @Correctness
  }
  else
  {
    tmp._u32 = (u16)accumulated;
    t->float64_value = tmp._f32; // @Correctness
  }

  return t;
}

static f64
_accumulate_fraction(Asset_Lexer *lexer)
{
  f64 accumulated = 0.0;
  s64 exp = 1;

  while (1)
  {
    int x = peek_char(lexer);
    if (!is_digit(x))
      break;

    eat_char(lexer);

    f64 digit = (f64)(x - '0');
    accumulated += (digit * pow(0.1, (f32)exp));
    exp += 1;
  }

  return accumulated;
}

/*
  0hXXXXXXXX: Hex-float
  0.123       float
  .123        float
  .           Should have been pushed as its own token before the function is called.
  1.          float
  1           int
  12_34__56   valid
  00          valid
  001         valid
*/
static Asset_Token *
push_number(Asset_Lexer *lexer)
{
  int c = peek_char(lexer);
  if (c == '0')
  {
    eat_char(lexer);
    int next = peek_char(lexer);
    if (next == 'h' || next == 'H')
    {
      eat_char(lexer);
      return push_hex_float(lexer);
    }
    else
    {
      unwind_char(lexer);
      // fallthrough
    }
  }

  Asset_Token *t = push_token(lexer, ASSET_TOKEN_NUMBER);

  // Integer part
  u64 accumulator = 0;
  while (1)
  {
    int x = peek_char(lexer);
    if (x == '_')
    {
      eat_char(lexer);
      continue;
    }

    if (!is_digit(x)) break;

    eat_char(lexer);

    accumulator *= 10;
    accumulator += (u64)(x - '0');
  }

  // Fractional part
  f64 fraction = 0.0;
  if (peek_char(lexer) == '.')
  {
    t->flags |= ASSET_TOKEN_FLAG_FLOAT;
    eat_char(lexer);
    fraction = _accumulate_fraction(lexer);
  }

  if (t->flags & ASSET_TOKEN_FLAG_FLOAT)
    t->float64_value = (f64)accumulator + fraction;
  else
    t->integer_value = accumulator;

  return t;
}

static Asset_Token *
push_identifier_or_keyword(Asset_Lexer *lexer)
{
  eat_char(lexer);

  String s;
  s.str = lexer->input.str + lexer->input_cursor - 1;

  for(;;)
  {
    int c = peek_char(lexer);
    if (!_continues_identifier(c))
      break;
    eat_char(lexer);
  }

  s.len = lexer->input.str + lexer->input_cursor - s.str;

#if 0
  switch (s.len)
  {
    case 2:
    {
      if (string_equal(s, S("if")))  
        return push_token(lexer, TOK_KEYWORD_IF);
    }

    case 3:
    {
      if (string_equal(s, S("for")))  
        return push_token(lexer, TOK_KEYWORD_FOR);
    }

    case 4:
    {
      if (string_equal(s, S("else")))  
        return push_token(lexer, TOK_KEYWORD_ELSE);
    }
  }
#endif

  Asset_Token *t = push_token(lexer, ASSET_TOKEN_IDENTIFIER);
  t->string_value = s;
  return t;
}

//
// @Todo: We are not allowing escape double quote at the moment.
//
static String 
_get_string(Asset_Lexer *lexer)
{
  R_ASSERT(peek_char(lexer) == '"');
  eat_char(lexer);

  s64 s0 = lexer->input_cursor;
  s64 s1;

  int c = peek_char(lexer);
  while (1)
  {
    if (c == '"')
    {
      s1 = lexer->input_cursor;
      break;
    }

    eat_char(lexer);
    c = peek_char(lexer);
  }
  eat_char(lexer);

  s64 count = s1 - s0;

  String s = {lexer->input.str + s0, count};
  return s;
}

static Asset_Token *
push_string(Asset_Lexer *lexer)
{
  Asset_Token *t = push_token(lexer, ASSET_TOKEN_STRING);
  t->string_value = _get_string(lexer);
  return t;
}

static Asset_Token *
push_reference(Asset_Lexer *lexer)
{
  Asset_Token *t = push_token(lexer, ASSET_TOKEN_REF);
  String resolved_name;
  String referent = _get_string(lexer);
  if (referent.str[0] == '.')
  {
    String referrer = lexer->name;
    String strings[2] = {path_strip_filename(referrer), referent};
    String joined = path_join(strings, 2, '/', tctx.temp);

    String absolute_path = get_absolute_path(joined, tctx.temp);
    replace_in_place(&absolute_path, '\\', '/');
    R_ASSERT(begins_with(absolute_path, shared->data_path));
    absolute_path.str += shared->data_path.len;
    absolute_path.len -= shared->data_path.len;
    absolute_path = trim_left(absolute_path, S("/\\"));

    resolved_name = absolute_path;
  }
  else
  {
    resolved_name = referent;
  }

  t->guid_value = guid_from_string(resolved_name);
  return t;
}

b32
asset_tokenize(Asset_Lexer *lexer, 
               Guid         id,
               Allocator    allocator)
{
  String input = read_entire_file(asset_name_from_guid(id), allocator);

  lexer->id               = id;
  lexer->name             = asset_name_from_guid(id);
  lexer->input            = input;
  lexer->input_cursor     = 0;
  lexer->line             = 0;
  lexer->tokens.allocator = allocator;
  lexer->token_cursor     = 0;

  while (1)
  {
    int c = peek_char(lexer);

    if (c == -1 /* || c == 0 */) // no null-terminate. 
    {
      push_token(lexer, ASSET_TOKEN_END_OF_INPUT);
      break;
    }

    if (_is_whitespace(c)) 
    {
      eat_char(lexer);
      continue;
    }

    if (_is_digit(c))
    {
      push_number(lexer);
      continue;
    }

    if (_starts_identifier(c))
    {
      push_identifier_or_keyword(lexer);
      continue;
    }

    switch (c)
    {
      case '/':
      {
        eat_char(lexer);
        if (peek_char(lexer) == '/')
        {
          eat_char(lexer);
          while (_continues_single_line_comment(lexer))
          {
            eat_char(lexer);
          }
        }
        else
        {
          push_token(lexer, (Asset_Token_Type)c);
        }
      }break;

      case '@':
      {
        eat_char(lexer);
        if (peek_char(lexer) == '"')
        {
          push_reference(lexer);
        }
        else
        {
          push_token(lexer, (Asset_Token_Type)c);
        }
      }break;

      case '"':
      {
        push_string(lexer);
      }break;

      default:
      {
        push_token(lexer, (Asset_Token_Type)c);
        eat_char(lexer);
      }break;
    }
  }

  return true;
}

void
asset_debug_print_tokens(Asset_Lexer *lexer)
{
  for (Asset_Token t : lexer->tokens)
  {
    if (t.type >= 0 && t.type <= 255)
    {
      // printf("%c\n", t.type);
    }
    else if (t.type == ASSET_TOKEN_IDENTIFIER)
    {
      printf("%.*s\n", (int)t.string_value.len, t.string_value.str);
    }
  }
}

Asset_Token *
peek_token(Asset_Lexer *lexer)
{
  R_ASSERT(lexer->token_cursor < (s64)lexer->tokens.count);
  return lexer->tokens.data + lexer->token_cursor;
}

void 
eat_token(Asset_Lexer *lexer)
{
  R_ASSERT(lexer->token_cursor < (s64)lexer->tokens.count);
  lexer->token_cursor += 1;
}


// Assume there's no sign and eat as an integer.
s64
eat_unsigned_int(Asset_Lexer *lexer)
{
  Asset_Token *t = peek_token(lexer);
  R_ASSERT(t->type == ASSET_TOKEN_NUMBER);
  R_ASSERT((t->flags & ASSET_TOKEN_FLAG_FLOAT) == 0);
  eat_token(lexer);
  return (s64)t->integer_value;
}

// Assume there's no sign and eat as an float.
f64
eat_unsigned_float(Asset_Lexer *lexer)
{
  Asset_Token *t = peek_token(lexer);
  R_ASSERT(t->type == ASSET_TOKEN_NUMBER);
  R_ASSERT(t->flags & ASSET_TOKEN_FLAG_FLOAT);
  eat_token(lexer);
  return t->float64_value;
}

// First check for '_'
s64 
eat_signed_int(Asset_Lexer *lexer)
{
  b32 sign = false;
  if (peek_token(lexer)->type == '-')
  {
    eat_token(lexer);
    sign = true;
  }

  s64 x = (s64)eat_unsigned_int(lexer);
  if (sign) x = -x;

  return x;
}

// First check for '-'
f64 
eat_signed_float(Asset_Lexer *lexer)
{
  b32 sign = false;
  if (peek_token(lexer)->type == '-')
  {
    eat_token(lexer);
    sign = true;
  }

  f64 f = (f64)eat_unsigned_float(lexer);
  if (sign) f = -f;

  return f;
}

m4x4
eat_m4x4(Asset_Lexer *lexer)
{
  m4x4 m;
  for (int i = 0; i < 16; ++i)
  {
    m.f[i] = eat_signed_float(lexer);
  }
  return m;
}

// Assume it's a string literal
String
eat_string(Asset_Lexer *lexer)
{
  Asset_Token *t = peek_token(lexer);
  R_ASSERT(t->type == ASSET_TOKEN_STRING);
  eat_token(lexer);
  return t->string_value;
}

Guid
eat_reference(Asset_Lexer *lexer)
{
  Asset_Token *t = peek_token(lexer);
  R_ASSERT(t->type == ASSET_TOKEN_REF);
  eat_token(lexer);
  return t->guid_value;
}
