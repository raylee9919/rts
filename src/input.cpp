// Copyright Seong Woo Lee. All Rights Reserved.

internal Input_Action *input_action_from_string(String name) 
{
    Input_Action *action = table_find_pointer( &input_system->action_table, name );
    return action;
}

internal Input_Sample input_sample_from_string(Input_State *state, String name) 
{
    Input_Sample sample = {};

    Input_Action *action = input_action_from_string(name);

    Assert( action );

    // @Temporary
    if (state->key_is_down[action->key]) {
        if (action->sample_kind == INPUT_SAMPLE_BOOLEAN) {
            sample.boolean = true;
        }
    }

    return sample;
}

internal void input_action_register(Input_Action action, String name)
{
    table_add(&input_system->action_table, name, action);
}

internal void input_action_unregister(String name)
{
    table_remove(&input_system->action_table, name);
}

internal void input_system_init(OS_Handle window) 
{
    Arena *arena = arena_alloc();
    input_system = push_struct(arena, Input_System);
    input_system->arena = arena;

    input_system->window = window;

    log(LOG_INFO, S("Intialized input system."));
}

internal void input_system_shutdown() 
{
    arena_release(input_system->arena);

    log(LOG_INFO, S("Shutdown input system."));
}

internal u32 input_string_hash(String str) 
{
    u64 hash = XXH3_64bits_withSeed(str.str, str.len, 0);
    return (u32)((hash & 0xffffffff) ^ (hash >> 32));
}

internal void input_process(Input_State *state)
{
    // Clear per-frame states
    memset(state->transition_count, 0, array_count(state->transition_count) * sizeof(state->transition_count[0]));

    list_for (os->first_event, event) {
        if (event->window == input_system->window) {

            if (event->kind == OS_EVENT_PRESS) {
                state->key_is_down[event->key] = true;
                state->transition_count[event->key] += 1;
            }

            if (event->kind == OS_EVENT_RELEASE) {
                state->key_is_down[event->key] = false;
                state->transition_count[event->key] += 1;
            }
        }
    }
}
