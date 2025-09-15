/* =====================================v3===================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include <iostream>
#include <string>
#include <filesystem>

#include <stdio.h>
#include <stdlib.h>

namespace fs = std::filesystem;

// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"

// @Note: [.cpp]
#include "base/rts_base_inc.cpp"

#define ENTITY_DIRECTORY "../src/entity"
#define MAX_BUFFER_LENGTH 64 

// -------------------------------------
// @Note: Terminal Color. Use with %s
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"


internal void
meta_print_ok(void)
{
    printf("[%sOK%s] ", KGRN, KNRM);
}

internal void
meta_print_path(const char *path)
{
    printf("'%s%s%s'\n", KCYN, path, KNRM);
}

internal Buffer
read_entire_file(const char *filepath) 
{
    Buffer result = {};
    FILE *file = fopen(filepath, "rb");
    if (file) 
    {
        fseek(file, 0, SEEK_END);
        umm filesize = ftell(file);
        fseek(file, 0, SEEK_SET);
        result.data = (u8 *)malloc(filesize);
        result.count = filesize;
        Assert(fread(result.data, filesize, 1, file) == 1);
        fclose(file);
    }
    else 
    {
        printf("[ERROR] Couldn't open file %s.\n", filepath);
    }
    return result;
}

enum Member_Type {
    TYPE_INTERGER,
    TYPE_FLOAT,
    TYPE_QUTERNION,
    TYPE_V3,
};
struct Entity_Member {
    Member_Type type;

    char ident[64];
    unsigned int len;
};
struct Entity {
    char name[64];

    Entity_Member members[64];
    unsigned int membercount;
};
struct Entity_List {
    Entity *entities;
    unsigned int capacity;
    unsigned int count;

    Entity *base_entity;
};

struct Stream {
    unsigned int cursor;
    Buffer input;
    char *entityname;
};

void generate_entity_include_header() {
    const char *generatedfilename = "../src/generated/entity.h";
    FILE *file = fopen(generatedfilename, "wb");
    Assert(file);
    fprintf(file, "#include \"entity/entity_Entity.h\"\n");
    for (const auto &iter : fs::directory_iterator(ENTITY_DIRECTORY)) {
        const char *filepath = iter.path().generic_string().c_str();
        char *filename = (char *)get_filename_from_filepath(filepath);
        if (!string_equal(filename, "entity_Entity.h")) {
            fprintf(file, "#include \"entity/%s\"\n", filename);
        }
    }
    fclose(file);

    meta_print_ok();
    printf("Generated entity include header file in ");
    meta_print_path(generatedfilename);
}

// @All files are spec'ed to have underscore after 'entity'.
// e.g. entity_Camera.h
char *entity_from_filepath(const char *filepath) {
    char *filename = (char *)get_filename_from_filepath(filepath);

    char *result;

    char *begin = 0;
    char *end = 0;
    for (char *at = filename; *at; ++at) {
        if (*at == '_') {
            begin = at + 1;
        } else if (*at == '.') {
            end = at;
        }
    }
    Assert(begin && end);
    unsigned int len = end - begin;
    result = new char[len + 1];
    memory_copy(result, begin, len);
    result[len] = 0;

    return result;
}

void init(Stream *stream, Buffer input, const char *filepath) {
    stream->cursor = 0;
    stream->input = input;
    stream->entityname = entity_from_filepath(filepath);
}

int peek(Stream *stream) {
    if (stream->cursor < stream->input.count) {
        return stream->input.data[stream->cursor];
    } else {
        return -1;
    }
}

void eat(Stream *stream) {
    Assert(stream->cursor < stream->input.count);
    ++stream->cursor;
}

bool continues_identifier(int c) {
    if (is_alnum(c)) return true;
    if (c == '_') return true;
    return false;
}

char *lex_identifier(Stream *stream) {
    char *buffer = new char[MAX_BUFFER_LENGTH];
    unsigned int len = 0;

    int c = peek(stream);
    while (continues_identifier(c)) {
        Assert(len < MAX_BUFFER_LENGTH);
        buffer[len++] = c;
        eat(stream);
        c = peek(stream);
    }
    buffer[len] = 0;

    return buffer;
}

void eat_whitespace(Stream *stream) {
    while (stream->cursor < stream->input.count) {
        int c = peek(stream);
        if (is_whitespace(c)) {
            eat(stream);
        } else {
            break;
        }
    }
}

void begin_entity(Stream *stream) {
    while (stream->cursor < stream->input.count) {
        int c = peek(stream);
        if (c == 'B') {
            char *buf = lex_identifier(stream);
            scope_exit(delete [] buf); // Works on early return too.

            if (string_equal(buf, "BEGIN_ENTITY")) {
                return;
            }
        } else {
            eat(stream);
        }
    }
}

inline bool string_equals_integer_type(char *string) {
    return (string_equal(string, "s8") || string_equal(string, "s16") || string_equal(string, "s32") || string_equal(string, "s64") ||
            string_equal(string, "u8") || string_equal(string, "u16") || string_equal(string, "u32") || string_equal(string, "u64"));
}

inline bool string_equals_float_type(char *string) {
    return (string_equal(string, "f8") || string_equal(string, "f16") || string_equal(string, "f32") || string_equal(string, "f64"));
}

void parse_integer(Entity *entity, Stream *stream) {
    char *ident = lex_identifier(stream);
    scope_exit(delete [] ident);

    eat_whitespace(stream);
    int c = peek(stream);
    Assert(c == ';');
    Assert(entity->membercount < array_count(entity->members));
    Entity_Member *member = entity->members + entity->membercount++;

    unsigned int len = string_length(ident);
    Assert(len < array_count(member->ident));
    memory_copy(member->ident, ident, len);
    member->ident[len] = 0;

    member->type = TYPE_INTERGER;
}

void parse_float(Entity *entity, Stream *stream) {
    char *ident = lex_identifier(stream);
    scope_exit(delete [] ident);

    eat_whitespace(stream);
    int c = peek(stream);
    Assert(c == ';');
    Assert(entity->membercount < array_count(entity->members));
    Entity_Member *member = entity->members + entity->membercount++;

    unsigned int len = string_length(ident);
    Assert(len < array_count(member->ident));
    memory_copy(member->ident, ident, len);
    member->ident[len] = 0;

    member->type = TYPE_FLOAT;
}

void parse_quaternion(Entity *entity, Stream *stream) {
    char *ident = lex_identifier(stream);
    scope_exit(delete [] ident);

    eat_whitespace(stream);
    int c = peek(stream);
    Assert(c == ';');
    Assert(entity->membercount < array_count(entity->members));
    Entity_Member *member = entity->members + entity->membercount++;

    unsigned int len = string_length(ident);
    Assert(len < array_count(member->ident));
    memory_copy(member->ident, ident, len);
    member->ident[len] = 0;

    member->type = TYPE_QUTERNION;
}

void parse_v3(Entity *entity, Stream *stream) {
    char *ident = lex_identifier(stream);
    scope_exit(delete [] ident);

    eat_whitespace(stream);
    int c = peek(stream);
    Assert(c == ';');
    Assert(entity->membercount < array_count(entity->members));
    Entity_Member *member = entity->members + entity->membercount++;

    unsigned int len = string_length(ident);
    Assert(len < array_count(member->ident));
    memory_copy(member->ident, ident, len);
    member->ident[len] = 0;

    member->type = TYPE_V3;
}

void fill(Entity_List *entitylist, Stream *stream) {
    Assert(entitylist->count < entitylist->capacity);
    Entity *entity = entitylist->entities + entitylist->count++;
    unsigned int len = string_length(stream->entityname);
    Assert(len < array_count(entity->name));
    memory_copy(entity->name, stream->entityname, len);
    entity->name[len] = 0;

    if (string_equal(entity->name, "Entity")) {
        entitylist->base_entity = entity;
    }

    begin_entity(stream);

    bool stop = false;
    while (stream->cursor < stream->input.count && !stop) {
        int c = peek(stream);

        switch (c) {
            case 's':
            case 'u':
            {
                char *buf = lex_identifier(stream);
                scope_exit(delete [] buf);
                if (string_equals_integer_type(buf)) {
                    eat_whitespace(stream);
                    parse_integer(entity, stream);
                }
            } break;

            case 'f':
            {
                char *buf = lex_identifier(stream);
                scope_exit(delete [] buf);
                if (string_equals_float_type(buf)) {
                    eat_whitespace(stream);
                    parse_float(entity, stream);
                }
            } break;

            case 'v':
            {
                char *buf = lex_identifier(stream);
                scope_exit(delete [] buf);
                if (string_equal(buf, "v3")) {
                    eat_whitespace(stream);
                    parse_v3(entity, stream);
                }
            } break;

            default:
            {
                if (c == 'E') {
                    char *buf = lex_identifier(stream);
                    scope_exit(delete [] buf);
                    if (string_equal(buf, "END_ENTITY")) {
                        stop = true;
                    }
                } else if (c == 'Q') {
                    char *buf = lex_identifier(stream);
                    scope_exit(delete [] buf);
                    if (string_equal(buf, "Quaternion")) {
                        eat_whitespace(stream);
                        parse_quaternion(entity, stream);
                    }
                } else {
                    eat(stream);
                }
            } break;
        }
    }
}

void serialize_integer(char *ident, FILE *file) {
    fprintf(file, R"===(        fprintf(file, R"entity(; %s
    %%u
)entity", e->%s);

)===", ident, ident);
}

void serialize_float(char *ident, FILE *file) {
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
)ENTITY", e->%s, to_raw(e->%s));

)===", ident, ident, ident);
}

void serialize_v3(char *ident, FILE *file) {
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
)ENTITY", e->%s.x, to_raw(e->%s.x), e->%s.y, to_raw(e->%s.y), e->%s.z, to_raw(e->%s.z));

)===", ident, ident, ident, ident, ident, ident, ident);
}

void serialize_quaternion(char *ident, FILE *file) {
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
)ENTITY", e->%s.w, to_raw(e->%s.w), e->%s.x, to_raw(e->%s.x), e->%s.y, to_raw(e->%s.y), e->%s.z, to_raw(e->%s.z));

)===", ident, ident, ident, ident, ident, ident, ident, ident, ident);
}

void generate_entity_serialization_functions(Entity_List *entitylist, const char *generated_filepath) {
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (unsigned int i = 0; i < entitylist->count; ++i) {
        Entity *entity = entitylist->entities + i;

        fprintf(file, R"===(internal ENTITY_FUNCTION_SERIALIZE(serialize_%s) 
{
    if (entity->id != 0) {

        %s *e = (%s *)entity;

)===", entity->name, entity->name, entity->name);

        if (!string_equal(entity->name, "Entity")) {
            fprintf(file, R"===(        fprintf(file, "\n%s\n");
        fprintf(file, "1\n");
        fprintf(file, "%%u\n", e->id);

        e->serialize_entity(entity, game_state, file);
)===", entity->name);
        }

        for (unsigned int j = 0; j < entity->membercount; ++j) {
            Entity_Member *member = entity->members + j;

            switch (member->type) {
                case TYPE_INTERGER:
                serialize_integer(member->ident, file);
                break;

                case TYPE_FLOAT:
                serialize_float(member->ident, file);
                break;

                case TYPE_V3:
                serialize_v3(member->ident, file);
                break;

                case TYPE_QUTERNION:
                serialize_quaternion(member->ident, file);
                break;
                            
                INVALID_DEFAULT_CASE;
            }
        }
        fprintf(file, "    }\n} // %s\n\n", entity->name);
    }

    fclose(file);
    meta_print_ok();
    printf("Generated entity serialization functions in ");
    meta_print_path(generated_filepath);
}

void generate_entity_parse_inline(Entity_List *entitylist, const char *generated_filepath) {
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (unsigned int i = 0; i < entitylist->count; ++i) {
        Entity *entity = entitylist->entities + i;
        if (!string_equal(entity->name, "Entity")) {
            fprintf(file, "PARSE_ENTITY(%s);\n", entity->name);
        }
        for (unsigned int j = 0; j < entity->membercount; ++j) {
            Entity_Member *member = entity->members + j;
        }
    }

    fclose(file);

    meta_print_ok();
    printf("Generated entity parse inline codes in ");
    meta_print_path(generated_filepath);
}

void deserialize_integer(FILE *file, char *ident) {
    fprintf(file, R"===(
        e->%s = parse_integer(parser);
)===", ident);
}

void deserialize_float(FILE *file, char *ident) {
    fprintf(file, R"===(
        e->%s = parse_float(parser);
)===", ident);
}

void deserialize_v3(FILE *file, char *ident) {
    fprintf(file, R"===(
        e->%s.x = parse_float(parser);
        e->%s.y = parse_float(parser);
        e->%s.z = parse_float(parser);
)===", ident, ident, ident);
}

void deserialize_quaternion(FILE *file, char *ident) {
    fprintf(file, R"===(
        e->%s.w = parse_float(parser);
        e->%s.x = parse_float(parser);
        e->%s.y = parse_float(parser);
        e->%s.z = parse_float(parser);
)===", ident, ident, ident, ident);
}

void deserialize_member(FILE *file, Entity_Member *member) {
    fprintf(file, R"===(
    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "%s")) {
        eat_token(parser);
)===", member->ident);

    switch (member->type) {
        case TYPE_INTERGER:
        deserialize_integer(file, member->ident);
        break;

        case TYPE_FLOAT:
        deserialize_float(file, member->ident);
        break;

        case TYPE_V3:
        deserialize_v3(file, member->ident);
        break;

        case TYPE_QUTERNION:
        deserialize_quaternion(file, member->ident);
        break;

        INVALID_DEFAULT_CASE;
    }

    fprintf(file, R"===(
        return;
    } 
)===");
}

void deserialize_entity(FILE *file, Entity *entity) {
    for (unsigned int j = 0; j < entity->membercount; ++j) {
        Entity_Member *member = entity->members + j;
        deserialize_member(file, member);
    }
}

void generate_entity_deserialization_functions(Entity_List *entitylist, const char *generated_filepath) {
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (unsigned int i = 0; i < entitylist->count; ++i) {
        Entity *entity = entitylist->entities + i;

        if (!string_equal(entity->name, "Entity")) {

            fprintf(file, R"===(
void parse_member_%s(%s *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);
)===", entity->name, entity->name);

            deserialize_entity(file, entitylist->base_entity);
            deserialize_entity(file, entity);

            if (entity->membercount != 0) {
                fprintf(file, R"===(
    else {
        INVALID_CODE_PATH;
    }
)===");
            } fprintf(file, "}\n");

            fprintf(file, R"===(
internal void
parse_%s(Parser *parser) 
{
    %s *e = push_entity(parser->game_state->world, %s, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_%s(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}
)===", entity->name, entity->name, entity->name, entity->name);
            }
    }

    fclose(file);

    meta_print_ok();
    printf("Generated entity deserialization functions in ");
    meta_print_path(generated_filepath);
}

void generate_entity_functions() {
    Entity_List entitylist;
    entitylist.count = 0;
    entitylist.capacity = 1024;
    entitylist.entities = new Entity[entitylist.capacity];
    scope_exit(delete [] entitylist.entities);

    for (const auto &iter : fs::directory_iterator(ENTITY_DIRECTORY)) {
        const char *filepath = iter.path().generic_string().c_str();
        Buffer input = read_entire_file(filepath);
        Stream stream = {};
        init(&stream, input, filepath);
        fill(&entitylist, &stream);
        delete [] stream.entityname;
    }

#if 0 // @To check if the list is filled out as desired.
    for (unsigned int i = 0; i < entitylist.count; ++i) {
        Entity *entity = entitylist.entities + i;
        for (unsigned int j = 0; j < entity->membercount; ++j) {
            Entity_Member *member = entity->members + j;
            printf("%u %s\n", member->type, member->ident);
        }
    }
#endif

    generate_entity_serialization_functions(&entitylist, "../src/generated/entity_serialization.h");
    generate_entity_parse_inline(&entitylist, "../src/generated/entity_parse.inl" );
    generate_entity_deserialization_functions(&entitylist, "../src/generated/entity_deserialization.h");


    meta_print_ok();
    printf("Successfully written entity functions.\n");
}

int main(void)
{
    generate_entity_include_header();
    generate_entity_functions();


    meta_print_ok();
    printf("Metaprogramming done.\n\n");
    return 0;
}
