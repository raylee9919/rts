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
    UI_SIZE_TYPE_PCT,

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
    UI_BOX_FLAG_DRAW_HOT_EFFECT    = (1<<5),
    UI_BOX_FLAG_DRAW_ACTIVE_EFFECT = (1<<6),
};

struct Ui_Text
{
    Utf8    string;
    AABB2   aabb;
    f32     padding;
};

struct Ui_Box
{
    // Links
    Ui_Box         *parent;
    Ui_Box         *first;
    Ui_Box         *last;
    Ui_Box         *next;
    Ui_Box         *prev;
    Ui_Box         *hash_next;

    Ui_Key          key;
    Ui_Box_Flags    flags;

    Axis2           flow;

    Ui_Size         semantic_size[AXIS2_COUNT];
    f32             computed_size[AXIS2_COUNT];
    f32             relative_position[AXIS2_COUNT];
    f32             position[AXIS2_COUNT];

    // Style
    v4 bg;
    f32 corner_radius00, corner_radius01, corner_radius10, corner_radius11;


    // Equipment
    Ui_Text        *text;


    // Animation time
    f32             hot_t;
    f32             active_t;
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

struct Ui_Style
{
    Ui_Style *next;

    union {
        v4 bg;
        Axis2 flow;
        f32 text_padding;
        f32 corner_radius00;
        f32 corner_radius01;
        f32 corner_radius10;
        f32 corner_radius11;
        Ui_Size size;
    };
};

struct Ui_State
{
    Arena          *permanent_arena;


    Arena          *build_arena[2];
    u64             tick;


    u64             box_table_size;
    Ui_Box_Slot    *box_table;


    Ui_Box          *first_free_box;
    Ui_Box          *last_free_box;



    f32             dt;



    Ui_Box          *root;

    Face            *face;
    Render_Id        texture_id;


    Ui_Box          *current_parent;

    Ui_Style        *bg_first;
    Ui_Style        *flow_first;
    Ui_Style        *text_padding_first;
    Ui_Style        *corner_radius00_first;
    Ui_Style        *corner_radius01_first;
    Ui_Style        *corner_radius10_first;
    Ui_Style        *corner_radius11_first;
    Ui_Style        *size_first[AXIS2_COUNT];

    Ui_Key           hot_key;
    Ui_Key           active_key;
};


// # Note: Constants
//
global read_only Ui_Key ui_key_zero = {};


// # Note: Function Delcarations.
//

internal Ui_State      *ui_alloc(void);
internal void           ui_init(Ui_State *ui);

internal void           ui_begin(f32 dt);
internal void           ui_end(void);

internal void           ui_parent_push(Ui_Box *box);
internal void           ui_parent_pop(void);

internal Ui_Box        *ui_box_alloc(void);
internal Ui_Box        *ui_box_build_from_string(Ui_Box_Flags flags, Utf8 string);
internal Ui_Box        *ui_box_build_from_key(Ui_Box_Flags flags, Ui_Key key);

internal void           ui_equip_text(Ui_Box *box, Utf8 text);

internal void           ui_solve_size_independent(Ui_Box *root, Axis2 axis);
internal void           ui_solve_size_dependent_upward(Ui_Box *root, Axis2 axis);
internal void           ui_solve_size_dependent_downward(Ui_Box *root, Axis2 axis);
internal void           ui_solve_size_violation(Ui_Box *root, Axis2 axis);

internal void           ui_draw(Ui_Box *child, v2 root_position);

internal Ui_Signal      ui_signal_from_box(Ui_Box *box);

internal b32            ui_key_match(Ui_Key a, Ui_Key b);
internal Ui_Key         ui_key_from_string(Utf8 string);
internal b32            ui_box_is_nil(Ui_Box *box);
internal Ui_Box        *ui_box_from_key(Ui_Key key);
internal Arena         *ui_build_arena(void);


// @Note: Style operations.
//
#define ui_style_push(name, val) {\
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);\
    style->name = val;\
    stack_push(ui_state->name##_first, style); }
#define ui_style_pop(name) {\
    stack_pop(ui_state->name##_first); }
#define ui_style_top(name) (ui_state->name##_first->name)

internal void ui_size_push(Axis2 axis, Ui_Size_Type type, f32 value);
internal void ui_size_pop(Axis2 axis);
internal Ui_Size ui_size_top(Axis2 axis);






#endif // RTS_UI_CORE_H
