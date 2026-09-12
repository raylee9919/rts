// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INPUT_H
#define RTS_INPUT_H

#include "basic/core.h"
#include "basic/arena.h"
#include "basic/hash_table.h"
#include "basic/string.h"
#include "math/math.h"
#include "os/os.h"

enum Input_Sample_Kind : u16 {
    INPUT_SAMPLE_BOOLEAN,
    INPUT_SAMPLE_SCALAR,
    INPUT_SAMPLE_VECTOR2,
    INPUT_SAMPLE_VECTOR3
};

struct Input_Sample {
    Input_Sample_Kind kind;
    union {
        b8  boolean;
        f32 scalar;
        v2  vector2;
        v3  vector3;
    };
};

struct Input_Action {
    OS_Key              key;
    Input_Sample_Kind   sample_kind;
};


u32 input_string_hash(String str);

struct Input_State {
    b8  key_is_down[KEY_GOOD_CAP];
    u16 transition_count[KEY_GOOD_CAP];
};

struct Input_System {
    Arena       *arena;
    OS_Handle   window;
    Table <String, Input_Action, input_string_hash> action_table;
};
extern Input_System *input_system;


void          input_system_init(OS_Handle window);
void          input_system_shutdown();

void          input_action_register(Input_Action action, String name);
void          input_action_unregister(String name);
Input_Action *input_action_from_string(String name);
Input_Sample  input_sample_from_string(Input_State *state, String name);

void          input_process(Input_State *state);


#endif // RTS_INPUT_H
