// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SERIALIZER_H
#define RTS_SERIALIZER_H


// https://handmade.network/p/29/swedish-cubes-for-unity/blog/p/2723-how_media_molecule_does_serialization


#define SRLZ_ADD(_FieldAdded, _Data, _FieldName)    \
    if ((s)->version >= (_FieldAdded)) {            \
        serialize((s), &((_Data)->_FieldName));     \
    }

#define SRLZ_REM(_AddedVersion, _RemovedVersion, _Type, _FieldName, _DefaultValue)  \
    _Type _FieldName = (_DefaultValue);                                             \
    if ((s)->version >= (_AddedVersion) && (s)->version < (_RemovedVersion)) {      \
        serialize((s), &(_FieldName));                                              \
    }


enum Data_Version : s32 {
    VER_0 = 0,
    VER_LATEST
};

struct Serializer {
    Data_Version    version;
    u8              *base;
    u8              *cursor;
    b32             is_writing;
};

internal void serializer_init(Serializer *s, Data_Version version, void *ptr, b32 is_writing);

#endif // RTS_SERIALIZER_H
