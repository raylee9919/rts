// Copyright Seong Woo Lee. All Rights Reserved.

#include "./input.h"
#include "shared.h"

void 
input_process(Input_State *state)
{
  u8 index = (state->index + 1) % 2;

  // Clear for this frame.
  memset(state->num_transitions, 0, sizeof(state->num_transitions));
  memcpy(state->ended_down[index], state->ended_down[state->index], sizeof(b8)*KEY_CODE_MAX);
  state->wheel_notches = 0;
  state->index = index;

  for (Key_Code key = 0; key < array_count(os->input_button_states); ++key)
  {
    if (os->input_button_states[key] & KEY_STATE_DOWN)
    {
      if (os->input_button_states[key] & KEY_STATE_START)
      {
        state->ended_down[state->index][key] = 1;
        state->num_transitions[key] += 1;
      }
      else if (os->input_button_states[key] & KEY_STATE_END)
      {
        state->ended_down[state->index][key] = 0;
        state->num_transitions[key] += 1;
      }
    }
    else
    {
      state->ended_down[state->index][key] = 0;
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
  return state->ended_down[state->index][key];
}

b8
key_up(Input_State *state, Key_Code key)
{
  return !state->ended_down[state->index][key];
}

b8
key_transitioned_down(Input_State *state, Key_Code key)
{
  u8 other_index = (state->index + 1) % 2;
  return state->ended_down[state->index][key] && !state->ended_down[other_index][key];
}

b8
key_transitioned_up(Input_State *state, Key_Code key)
{
  u8 other_index = (state->index + 1) % 2;
  return !state->ended_down[state->index][key] && state->ended_down[other_index][key];
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
