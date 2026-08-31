// Copyright Seong Woo Lee. All Rights Reserved.

internal void serializer_init(Serializer *s, Data_Version version, void *ptr, b32 is_writing) {
    Construct(s);
    s->version      = version;
    s->base         = (u8*)ptr;
    s->cursor       = (u8*)ptr;
    s->is_writing   = is_writing;
}

internal void serialize(Serializer *s, String str) {
    u64 sz = sizeof(str.str[0]) * str.len;
    if (s->is_writing)  memcpy(s->cursor, str.str, sz);
    else                memcpy(str.str, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, s8 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, s16 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, s32 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, s64 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, u8 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, u16 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, u32 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, u64 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, f32 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, f64 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, v2 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, v3 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, v4 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, m4x4 *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, Guid *data) {
    u64 sz = sizeof(*data);
    if (s->is_writing)  memcpy(s->cursor, data, sz);
    else                memcpy(data, s->cursor, sz);
    s->cursor += sz;
}

internal void serialize(Serializer *s, Entity *entity) 
{
    SRLZ_ADD( VER_0, entity, position );
    SRLZ_ADD( VER_0, entity, mesh );
    SRLZ_ADD( VER_0, entity, material );
}
