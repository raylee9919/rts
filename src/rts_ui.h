#ifndef RTS_UI_H
#define RTS_UI_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// # Note: core.
//
struct Ui_Key
{
    u64 e[1];
};

enum Ui_Size_Type
{
    UI_SIZE_NULL = 0,
    UI_SIZE_PX   = 1,
};

struct Ui_Size
{
    Ui_Size_Type    type;
    f32             value;
    f32             strictness;
};

typedef u32 Ui_Box_Flags;
enum
{
    UI_BOX_FLAG_CLICKABLE       = (1<<0),
    UI_BOX_FLAG_DRAW_BACKGROUND = (1<<1),
};

struct Ui_Box
{
    // # Note: Box hierarchy
    //         Rewritten from scratch on every frame.
    Ui_Box          *parent;
    Ui_Box          *first;
    Ui_Box          *last;
    Ui_Box          *next;
    Ui_Box          *prev;

    // # Note: Hash table chain (closed addressing/open hashing)
    //         Used to lookup the persistent part of the structure.
    Ui_Box          *hash_next;


    Ui_Box_Flags    flags;

    Ui_Key          key;

    f32             t_hot;
    f32             t_active;
};

struct Ui_Signal
{
    Ui_Box  *box;
    b8      clicked;
};

struct Ui_Box_Slot
{
    Ui_Box *first;
    Ui_Box *last;
};

struct Ui_State
{
    Arena          *arena;

    Ui_Key          key_hot;
    Ui_Key          key_active;

    u64             box_table_size;
    Ui_Box_Slot    *box_table;
};


// # Note: globals.
//
global Ui_State *ui;
read_only Ui_Key ui_key_zero;


// # Note: core functions.
//








#endif // RTS_UI_H
