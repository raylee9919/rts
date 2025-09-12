/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

enum Mouse_Enum {
    Mouse_Left,
    Mouse_Middle,
    Mouse_Right,
    Mouse_Extended0,
    Mouse_Extended1,

    Mouse_Count
};
struct Mouse_Input {
    b32 is_down[Mouse_Count];
    b32 toggle[Mouse_Count];

    v2 position;
    v2 click_p[Mouse_Count];
    s32 wheel_delta;
};

enum Event_Flag : u8 {
    PRESSED  = 0x1,
    RELEASED = 0x2,
};
struct Event {
    u8 key;
    u8 flag;
};
struct Event_Queue {
    Event events[256];
    u32 next_idx;
};

struct Game_Key {
    b32 is_down;
    b32 toggled;
    f32 pressed_time;
};

struct Input {
    f32 actual_dt;
    f32 dt;

    v2u draw_dim;

    Game_Key keys[256];
    Mouse_Input mouse;
    v2 prev_mouse_p;

    b32 interacted_ui;
    
    u32 hot_entity_id;

    b32 quit_requested;
};
