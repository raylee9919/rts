#ifndef RTS_META_H
#define RTS_META_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

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

enum Member_Type 
{
    TYPE_INTERGER,
    TYPE_FLOAT,
    TYPE_QUTERNION,
    TYPE_V3,
};

struct Entity_Member 
{
    Member_Type type;

    u8 ident[64];
    u32 len;
};

struct Entity 
{
    u8 name[64];

    Entity_Member members[64];
    u16 membercount;
};

struct Entity_List 
{
    Entity *entities;
    u32 capacity;
    u32 count;

    Entity *base_entity;
};

struct Stream 
{
    u32 cursor;
    Utf8 input;
    char *entityname;
};

#endif // RTS_META_H
