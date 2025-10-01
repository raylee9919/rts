#ifndef RTS_UI_CORE_H
#define RTS_UI_CORE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

enum Ui_Size_Type
{
    UI_SIZE_TYPE_PX,
    UI_SIZE_TYPE_TEXT,
    UI_SIZE_TYPE_CHILDREN,

    UI_SIZE_TYPE_COUNT,
};

struct Ui_Size
{
    Ui_Size_Type  type;
    f32           value;
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
    UI_BOX_FLAG_DISABLED           = (1<<0),
    UI_BOX_FLAG_MOUSE_CLICKABLE    = (1<<1),
    UI_BOX_FLAG_KEYBOARD_CLICKABLE = (1<<2),

    // Note: Draw
    //
    UI_BOX_FLAG_DRAW_BACKGROUND    = (1<<3),
    UI_BOX_FLAG_DRAW_TEXT          = (1<<4),

    // Note: Layout
    //
    UI_BOX_FLAG_FLOW_X             = (1<<5),
    UI_BOX_FLAG_FLOW_Y             = (1<<6),

    // Note: Text Alignment
    //
    UI_BOX_FLAG_TEXT_ALIGN_CENTER  = (1<<7),
};

struct Ui_Box
{
    Ui_Box          *parent;
    Ui_Box          *first;
    Ui_Box          *last;
    Ui_Box          *next;
    Ui_Box          *prev;

    Ui_Box          *hash_next;

    Ui_Key          key;
    Ui_Box_Flags    flags;

    Ui_Size         semantic_size[2];
    v2              computed_size;

    v2              position;

    Utf8            text;
    AABB2           text_aabb;


    v4              bg[4]; // 00, 10, 01, 11

    f32             padding;
    f32             border;
    f32             margin;
};

struct Ui_Signal
{
    Ui_Box  *box;

    b32 pressed_left;
    b32 pressed_middle;
    b32 pressed_right;
    b32 released_left;
    b32 released_middle;
    b32 released_right;
    b32 clicked_left;
    b32 clicked_right;
    b32 clicked_middle;
    b32 dragging_left;
    b32 dragging_middl;
    b32 dragging_right;
    b32 double_clicked_left;
    b32 double_clicked_middle;
    b32 double_clicked_right;
    b32 pressed_key;
    b32 hovering;
    b32 mouse_is_over;
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

    Ui_Box          *first_free_box;
    Ui_Box          *last_free_box;


    Ui_Box          *root;
    Ui_Box          *current_parent;

    Face            *face;
    Render_Id        texture_id;
};


// # Note: Constants
//
global read_only Ui_Key ui_key_zero = {};


// # Note: Function Delcarations.
//

internal Ui_State *ui_alloc(void);
internal void ui_init(Ui_State *ui);

internal b32 ui_key_match(Ui_Key a, Ui_Key b);
internal Ui_Key ui_key_from_string(Utf8 string);
internal b32 ui_box_is_nil(Ui_Box *box);
internal Ui_Box *ui_box_from_key(Ui_Key key);

internal Ui_Box *ui_box_alloc(void);
internal Ui_Box *ui_push(Utf8 string, Ui_Box_Flags flags);
internal void ui_pop(void);


internal v2 ui_compute_size_internal(Ui_Box *box);
internal void ui_compute_position(Ui_Box *box, v2 position);
internal void ui_box_draw(Ui_Box *box);
internal Ui_Signal ui_signal_from_box(Ui_Box *box);







#endif // RTS_UI_CORE_H
