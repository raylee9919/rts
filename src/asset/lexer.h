// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_LEXER_H
#define RTS_ASSET_LEXER_H

#include "basic/string.h"
#include "basic/array.h"
#include "asset/asset.h"

/*
    Asset reference consists of a string enclosed by double quotes, prefixed by `@`.
    No spaces in-between.
    e.g. `@"path_to_material"`  is valid.
         `@ "path_to_material"` is invalid.
    
    Should I keep @ and "" as separate tokens? I dunno. 
    Based on my current understanding, I don't see a compelling reason 
    to separate them.
                                                  - SeongWoo 2026-10-12
*/

enum Asset_Token_Type : u16
{
  // Mentally insert ASCII here.

  ASSET_TOKEN_IDENTIFIER = 256,
  ASSET_TOKEN_NUMBER     = 257,
  ASSET_TOKEN_STRING     = 258,
  ASSET_TOKEN_REF        = 259,

  ASSET_TOKEN_KEYWORD_TRUE,
  ASSET_TOKEN_KEYWORD_FALSE,

  ASSET_TOKEN_ERROR,
  ASSET_TOKEN_END_OF_INPUT,
};

typedef u16 Asset_Token_Flags;
enum 
{
  ASSET_TOKEN_FLAG_FLOAT            = 0x1,
  ASSET_TOKEN_FLAG_REQUIRES_FLOAT64 = 0x2,
};

struct Asset_Token
{
  Asset_Token_Type  type;
  Asset_Token_Flags flags;
  union
  {
    String     string_value;
    u64        integer_value;
    f64        float64_value;
    Guid       guid_value;
  };
};

struct Asset_Lexer
{
  Guid               id;
  String             name;

  String             input;
  s64                input_cursor;

  s64                line;

  Array<Asset_Token> tokens;
  s64                token_cursor;
};

b32
asset_tokenize(Asset_Lexer *lexer, 
               Guid         id,
               Allocator    token_allocator);

Asset_Token *
peek_token(Asset_Lexer *lexer);

void 
eat_token(Asset_Lexer *lexer);


// Those procedures assumes the next token is 
// the type it is looking for, and naively consumes it. 
// If it is different from the expectation, asserts.
//
s64
eat_unsigned_int(Asset_Lexer *lexer);

f64
eat_unsigned_float(Asset_Lexer *lexer);

s64 
eat_signed_int(Asset_Lexer *lexer);

f64 
eat_signed_float(Asset_Lexer *lexer);

m4x4
eat_m4x4(Asset_Lexer *lexer);

String
eat_string(Asset_Lexer *lexer);

String
eat_identifier(Asset_Lexer *lexer);

Guid
eat_reference(Asset_Lexer *lexer);


#endif // RTS_ASSET_LEXER_H
