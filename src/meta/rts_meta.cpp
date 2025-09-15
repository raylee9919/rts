/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include <iostream>
#include <string>
#include <filesystem>

#include <stdio.h>
#include <stdlib.h>

namespace fs = std::filesystem;

// ------------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"

#include "rts_meta.h"

// ------------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"

#define ENTITY_DIRECTORY "../src/entity"
#define MAX_BUFFER_LENGTH 64 


Arena *arena;


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

internal void
generate_entity_include_header() 
{
    const char *generatedfilename = "../src/generated/entity.h";
    FILE *file = fopen(generatedfilename, "wb");
    Assert(file);
    fprintf(file, "#include \"entity/entity_Entity.h\"\n");
    for (const auto &iter : fs::directory_iterator(ENTITY_DIRECTORY)) 
    {
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

internal char *
entity_from_filepath(const char *filepath) 
{
    // @Note: All files are spec'ed to have underscore after 'entity'.
    //        e.g. entity_Camera.h

    char *filename = (char *)get_filename_from_filepath(filepath);

    char *result;

    char *begin = 0;
    char *end = 0;
    for (char *at = filename; *at; ++at) 
    {
        if (*at == '_') 
        { begin = at + 1; }
        else if (*at == '.') 
        { end = at; }
    }
    Assert(begin && end);
    u64 len = end - begin;
    result = new char[len + 1];
    memory_copy(result, begin, len);
    result[len] = 0;

    return result;
}

internal void
init(Stream *stream, Utf8 input, const char *filepath) 
{
    stream->cursor = 0;
    stream->input = input;
    stream->entityname = entity_from_filepath(filepath);
}

internal int
peek(Stream *stream) 
{
    if (stream->cursor < stream->input.len) 
    { return stream->input.str[stream->cursor]; }
    else 
    { return -1; }
}

internal void
eat(Stream *stream) 
{
    Assert(stream->cursor < stream->input.len);
    ++stream->cursor;
}

internal b32
continues_identifier(int c) 
{
    if (is_alnum(c)) return true;
    if (c == '_') return true;
    return false;
}

internal char *
lex_identifier(Stream *stream) 
{
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

internal void
eat_whitespace(Stream *stream) 
{
    while (stream->cursor < stream->input.len) 
    {
        int c = peek(stream);
        if (is_whitespace(c)) 
        { eat(stream); }
        else 
        { break; }
    }
}

internal void 
begin_entity(Stream *stream) 
{
    while (stream->cursor < stream->input.len) {
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

internal b32
string_equals_integer_type(char *string) 
{
    return (string_equal(string, "s8") || string_equal(string, "s16") || string_equal(string, "s32") || string_equal(string, "s64") ||
            string_equal(string, "u8") || string_equal(string, "u16") || string_equal(string, "u32") || string_equal(string, "u64"));
}

internal b32
string_equals_float_type(char *string) 
{
    return (string_equal(string, "f8") || string_equal(string, "f16") || string_equal(string, "f32") || string_equal(string, "f64"));
}

internal void
parse_integer(Entity *entity, Stream *stream) 
{
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

internal void
parse_float(Entity *entity, Stream *stream) 
{
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

internal void
parse_quaternion(Entity *entity, Stream *stream) 
{
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

internal void
parse_v3(Entity *entity, Stream *stream) 
{
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

internal void
fill(Entity_List *entitylist, Stream *stream) 
{
    Assert(entitylist->count < entitylist->capacity);
    Entity *entity = entitylist->entities + entitylist->count++;
    u64 len = string_length(stream->entityname);
    Assert(len < array_count(entity->name));
    memory_copy(entity->name, stream->entityname, len);
    entity->name[len] = 0;

    if (string_equal((char *)entity->name, "Entity")) 
    {
        entitylist->base_entity = entity;
    }

    begin_entity(stream);

    b32 stop = false;
    while (stream->cursor < stream->input.len && !stop) 
    {
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

internal void
serialize_integer(char *ident, FILE *file) 
{
    fprintf(file, R"===(        fprintf(file, R"entity(; %s
    %%u
)entity", e->%s);

)===", ident, ident);
}

internal void
serialize_float(char *ident, FILE *file) 
{
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
)ENTITY", e->%s, to_raw(e->%s));

)===", ident, ident, ident);
}

internal void
serialize_v3(char *ident, FILE *file) 
{
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
)ENTITY", e->%s.x, to_raw(e->%s.x), e->%s.y, to_raw(e->%s.y), e->%s.z, to_raw(e->%s.z));

)===", ident, ident, ident, ident, ident, ident, ident);
}

internal void
serialize_quaternion(char *ident, FILE *file) 
{
    fprintf(file, R"===(        fprintf(file, R"ENTITY(; %s
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
    %%.6f : 0x%%X
)ENTITY", e->%s.w, to_raw(e->%s.w), e->%s.x, to_raw(e->%s.x), e->%s.y, to_raw(e->%s.y), e->%s.z, to_raw(e->%s.z));

)===", ident, ident, ident, ident, ident, ident, ident, ident, ident);
}

internal void
generate_entity_serialization_functions(Entity_List *entitylist, const char *generated_filepath) 
{
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (unsigned int i = 0; i < entitylist->count; ++i) {
        Entity *entity = entitylist->entities + i;

        fprintf(file, R"===(internal ENTITY_FUNCTION_SERIALIZE(serialize_%s) 
{
    if (entity->id != 0) {

        %s *e = (%s *)entity;

)===", entity->name, entity->name, entity->name);

        if (! string_equal((char *)entity->name, "Entity")) 
        {
            fprintf(file, R"===(        fprintf(file, "\n%s\n");
        fprintf(file, "1\n");
        fprintf(file, "%%u\n", e->id);

        e->serialize_entity(entity, game_state, file);
)===", entity->name);
        }

        for (u64 j = 0; j < entity->membercount; ++j) 
        {
            Entity_Member *member = entity->members + j;

            switch (member->type) 
            {
                case TYPE_INTERGER:
                serialize_integer((char *)member->ident, file);
                break;

                case TYPE_FLOAT:
                serialize_float((char *)member->ident, file);
                break;

                case TYPE_V3:
                serialize_v3((char *)member->ident, file);
                break;

                case TYPE_QUTERNION:
                serialize_quaternion((char *)member->ident, file);
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

internal void
generate_entity_parse_inline(Entity_List *entitylist, const char *generated_filepath) 
{
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (u32 i = 0; i < entitylist->count; ++i) 
    {
        Entity *entity = entitylist->entities + i;
        if (! string_equal((char *)entity->name, "Entity")) 
        {
            fprintf(file, "PARSE_ENTITY(%s);\n", entity->name);
        }
        for (u32 j = 0; j < entity->membercount; ++j) 
        {
            Entity_Member *member = entity->members + j;
        }
    }

    fclose(file);

    meta_print_ok();
    printf("Generated entity parse inline codes in ");
    meta_print_path(generated_filepath);
}

internal void
deserialize_integer(FILE *file, char *ident) 
{
    fprintf(file, R"===(
        e->%s = parse_integer(parser);
)===", ident);
}

internal void
deserialize_float(FILE *file, char *ident) 
{
    fprintf(file, R"===(
        e->%s = parse_float(parser);
)===", ident);
}

internal void
deserialize_v3(FILE *file, char *ident) 
{
    fprintf(file, R"===(
        e->%s.x = parse_float(parser);
        e->%s.y = parse_float(parser);
        e->%s.z = parse_float(parser);
)===", ident, ident, ident);
}

internal void
deserialize_quaternion(FILE *file, char *ident) 
{
    fprintf(file, R"===(
        e->%s.w = parse_float(parser);
        e->%s.x = parse_float(parser);
        e->%s.y = parse_float(parser);
        e->%s.z = parse_float(parser);
)===", ident, ident, ident, ident);
}

internal void
deserialize_member(FILE *file, Entity_Member *member) 
{
    fprintf(file, R"===(
    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "%s")) {
        eat_token(parser);
)===", member->ident);

    switch (member->type) {
        case TYPE_INTERGER:
        deserialize_integer(file, (char *)member->ident);
        break;

        case TYPE_FLOAT:
        deserialize_float(file, (char *)member->ident);
        break;

        case TYPE_V3:
        deserialize_v3(file, (char *)member->ident);
        break;

        case TYPE_QUTERNION:
        deserialize_quaternion(file, (char *)member->ident);
        break;

        INVALID_DEFAULT_CASE;
    }

    fprintf(file, R"===(
        return;
    } 
)===");
}

internal void
deserialize_entity(FILE *file, Entity *entity) 
{
    for (unsigned int j = 0; j < entity->membercount; ++j) 
    {
        Entity_Member *member = entity->members + j;
        deserialize_member(file, member);
    }
}

internal void
generate_entity_deserialization_functions(Entity_List *entitylist, const char *generated_filepath) 
{
    FILE *file = fopen(generated_filepath, "wb");
    Assert(file);

    for (unsigned int i = 0; i < entitylist->count; ++i) {
        Entity *entity = entitylist->entities + i;

        if (! string_equal((char *)entity->name, "Entity")) {

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

internal void
generate_entity_functions() 
{
    Entity_List entitylist;
    {
        entitylist.count    = 0;
        entitylist.capacity = 1024;
        entitylist.entities = new Entity[entitylist.capacity];
    }
    scope_exit(delete [] entitylist.entities);

    for (const auto &iter : fs::directory_iterator(ENTITY_DIRECTORY)) 
    {
        const char *filepath = iter.path().generic_string().c_str();
        Utf8 input = read_entire_file(arena, utf8lit(filepath));
        Stream stream = {};
        init(&stream, input, filepath);
        fill(&entitylist, &stream);
        delete [] stream.entityname;
    }

#if 0 // @To check if the list is filled out as desired.
    for (unsigned int i = 0; i < entitylist.len; ++i) {
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
    os_init();
    thread_init();

    arena = arena_alloc();

    Utf8 art_path = {};
    Utf8 data_path = {};
    {
        Temporary_Arena scratch = scratch_begin();

        Utf8 binary_path = os.string_from_system_path_kind(scratch.arena, OS_SYSTEM_PATH_KIND_BINARY);
        Utf8 local_data_path = utf8f(scratch.arena, "%S/data", binary_path);
        Utf8 binary_parent_path = utf8_path_chop_last_slash(binary_path);
        Utf8 parent_data_path = utf8f(scratch.arena, "%S/data", binary_parent_path);

        Os_File_Attributes local_data_attr  = os.attributes_from_file_path(local_data_path);
        Os_File_Attributes parent_data_attr = os.attributes_from_file_path(parent_data_path);

        if (local_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
        { data_path = utf8_copy(arena, local_data_path); }
        else if (parent_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
        { data_path = utf8_copy(arena, parent_data_path); }

        scratch_end(scratch);
    }


    {
        generate_entity_include_header();
        generate_entity_functions();
    }


    {
        meta_print_ok();
        printf("Metaprogramming done.\n\n");
    }

    return 0;
}
