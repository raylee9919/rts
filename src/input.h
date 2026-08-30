// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INPUT_H
#define RTS_INPUT_H

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


internal u32 input_string_hash(String str);

struct Input_State {
    b8  key_is_down[KEY_GOOD_CAP];
    u16 transition_count[KEY_GOOD_CAP];
};

struct Input_System {
    Arena       *arena;
    OS_Handle   window;
    Table <String, Input_Action, input_string_hash> action_table; 
};
global Input_System *input_system;


internal void input_process(Input_State *state);


#endif // RTS_INPUT_H
