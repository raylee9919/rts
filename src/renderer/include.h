// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H


enum Render_Pass : u32 {
    RENDER_PASS_GEOMETRY = 0,
};

struct Render_Entry {

};

struct Render_SPSC_Queue {
    Semaphore       semaphore;
    s32             read_idx  = 0;
    s32             write_idx = 0;
    Render_Entry    entries[5]; // cap = 4

    b32 is_empty() {
        return read_idx == write_idx;
    }

    b32 is_full() {
        return (write_idx + 1) % array_count(entries) == read_idx;
    }

    s32 get_count() {
        if (write_idx > read_idx) {
            return write_idx - read_idx;
        } else {
            return array_count(entries) - (read_idx - write_idx);
        } 
    }

    void push(Render_Entry entry) {
        Assert(!is_full());
        entries[write_idx] = entry;
        write_idx += 1;
        write_idx %= array_count(entries);
        semaphore_signal(&semaphore);
    }

    Render_Entry pop() {
        Assert(!is_empty());
        Render_Entry entry = entries[read_idx];
        read_idx += 1;
        read_idx %= array_count(entries);
        return entry;
    }
};

global Render_SPSC_Queue render_queue;


#endif 
