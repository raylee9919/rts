// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INPUT_H
#define RTS_INPUT_H

struct Input_Action {
    OS_Key key;
};

struct Input_Value {
    b8 boolean;
    f32 x;
    f32 y;
    f32 z;
};

internal u32 input_string_hash(String str);

struct Input_State {
    b8  key_is_down[KEY_GOOD_CAP];
    u16 transition_count[KEY_GOOD_CAP];
};

#define FUCKYOU

struct Input_System {
    Arena       *arena;
    OS_Handle   window;
    Table <String, Input_Action, input_string_hash> action_table; 
};
global Input_System *input_system;


internal void input_process(Input_State *state);


#endif // RTS_INPUT_H
