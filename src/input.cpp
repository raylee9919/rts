/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



#define KEY_INPUT_REPEAT_THRESHOLD 0.4f
    

inline b32
toggled_down(Input *input, u8 idx) {
    return (input->keys[idx].toggled && input->keys[idx].is_down);
}

inline b32
toggled_up(Input *input, u8 idx) {
    return (input->keys[idx].toggled && !input->keys[idx].is_down);
}

inline b32
repeated(Input *input, u8 idx) {
    return (input->keys[idx].pressed_time >= KEY_INPUT_REPEAT_THRESHOLD);
}

inline b32
toggled_down_or_repeated(Input *input, u8 idx) {
    return (toggled_down(input, idx) || repeated(input, idx));
}

internal void
update_input_state(Input *input, Event_Queue *event_queue, f32 rdt) 
{
    for (u32 idx = 0; idx < array_count(input->keys); ++idx) {
        Game_Key *key = input->keys + idx;
        input->keys[idx].toggled = false;

        if (key->is_down)
            key->pressed_time += rdt;
    }

    while (event_queue->next_idx != 0) {
        Event ev = event_queue->events[--event_queue->next_idx];
        Game_Key *key = input->keys + ev.key;

        if (ev.flag & Event_Flag::PRESSED) {
            key->is_down = true;
        }
        else if (ev.flag & Event_Flag::RELEASED) {
            key->is_down = false;
            key->pressed_time = 0.0f;
        }

        key->toggled = true;
    }

    Assert(event_queue->next_idx == 0);
}
