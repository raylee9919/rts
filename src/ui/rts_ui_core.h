#ifndef RTS_UI_H
#define RTS_UI_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

enum Ui_Axis
{
    UI_AXIS_X,
    UI_AXIS_Y,
    UI_AXIS_COUNT,
};

enum Ui_Size_Type
{
    UI_SIZE_TYPE_PX,
    UI_SIZE_TYPE_TEXT,
    UI_SIZE_TYPE_CHILDREN,

    UI_SIZE_TYPE_COUNT,
};

struct Ui_Size
{
    Ui_Size_Type    type;
    f32             value;
};

struct Ui_Key
{
    u64 e[1];
};

typedef u32 Ui_Box_Flags;
enum
{
    // Note: Interact
    //
    UI_BOX_FLAG_DISABLED                = (1<<0),
    UI_BOX_FLAG_MOUSE_CLICKABLE         = (1<<1),
    UI_BOX_FLAG_KEYBOARD_CLICKABLE      = (1<<2),

    // Note: Draw
    //
    UI_BOX_FLAG_DRAW_BACKGROUND         = (1<<3),

    // Note: Layout
    //
    UI_BOX_FLAG_FLOW_X                  = (1<<4),
    UI_BOX_FLAG_FLOW_Y                  = (1<<5),
};

struct Ui_Box
{
    Ui_Box          *parent;
    Ui_Box          *first;
    Ui_Box          *last;
    Ui_Box          *next;
    Ui_Box          *prev;

    Ui_Box          *hash_next;

    Ui_Box_Flags    flags;
    Ui_Key          key;

    f32             computed_size[UI_AXIS_COUNT];
};

struct Ui_Signal
{
    Ui_Box  *box;
    b8       clicked;
};

struct Ui_Box_Slot
{
    Ui_Box *first;
    Ui_Box *last;
};

struct Ui_State
{
    Arena          *arena;

    Arena          *box_arena;
    u64             box_table_size;
    Ui_Box_Slot    *box_table;

    Ui_Box         *root;
};


// # Note: Constants
//
global read_only Ui_Key ui_key_zero = {};






#endif // RTS_UI_H
