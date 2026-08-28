// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H

#include "game.h"


struct Game_State;

enum Render_Pass : u32 {
    RENDER_PASS_GEOMETRY = 0,
};

struct Render_Entry {
    Mutex       mutex;
    Game_State  *game_state;
};

struct Render_SPSC_Queue {
    Mutex           mutex;
    Condvar         condvar;

    Render_Entry    entries[5]; // cap = 4
    s32             read_idx  = 0;
    s32             write_idx = 0;


    b32 is_empty() {
        return read_idx == write_idx;
    }

    b32 is_full() {
        return (write_idx + 1) % array_count(entries) == read_idx;
    }
};

global Render_SPSC_Queue render_queue;


#endif 
