// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INPUT_H
#define RTS_INPUT_H


#include "os/os.h"

struct Input_State
{
  b8  ended_down[KEY_CODE_MAX];
  s32 num_transitions[KEY_CODE_MAX];

  f32 wheel_notches;
};

void 
input_process(Input_State *state);

b8
key_down(Input_State *state, Key_Code key);

b8
key_up(Input_State *state, Key_Code key);

s32
key_presses(Input_State *state, Key_Code key);

s32
key_releases(Input_State *state, Key_Code key);


#endif // RTS_INPUT_H
