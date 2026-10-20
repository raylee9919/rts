// Copyright Seong Woo Lee. All Rights Reserved.

#include "./input.h"
#include "shared.h"

void 
input_process(Input_State *state)
{
  // Clear for this frame.
  // 'ended_down' is kept.
  memset(state->num_transitions, 0, sizeof(state->num_transitions));
  state->wheel_notches = 0;

  for (Key_Code key = 0; key < array_count(os->input_button_states); ++key)
  {
    if (os->input_button_states[key] & KEY_STATE_DOWN)
    {
      if (os->input_button_states[key] & KEY_STATE_START)
      {
        state->ended_down[key] = 1;
        state->num_transitions[key] += 1;
      }
      else if (os->input_button_states[key] & KEY_STATE_END)
      {
        state->ended_down[key] = 0;
        state->num_transitions[key] += 1;
      }
    }
    else
    {
      state->ended_down[key] = 0;
    }
  }

  for (Event& event : os->events) 
  {
    if (event.type == EVENT_KEYBOARD) 
    {
#if 1 // @Temporary
      if (event.key_code == KEY_ENTER && event.modifier_flags.alt_pressed && event.key_pressed) {
        toggle_fullscreen(shared->window);
      }

      if (event.key_code == KEY_F4 && event.modifier_flags.alt_pressed) {
        shared->should_close = true;
      }
#endif
    }
    else if (event.type == EVENT_MOUSE_WHEEL)
    {
      state->wheel_notches = (f32)event.wheel_delta / (f32)event.typical_wheel_delta;
    }
  }
}

b8
key_down(Input_State *state, Key_Code key)
{
  return state->ended_down[key];
}

b8
key_up(Input_State *state, Key_Code key)
{
  return !state->ended_down[key];
}

s32
key_presses(Input_State *state, Key_Code key)
{
  if (state->ended_down[key])
    return state->num_transitions[key] / 2 + 1;
  return state->num_transitions[key] / 2;
}

s32
key_releases(Input_State *state, Key_Code key)
{
  if (state->ended_down[key])
    return state->num_transitions[key] / 2;
  return state->num_transitions[key] / 2 + 1;
}
