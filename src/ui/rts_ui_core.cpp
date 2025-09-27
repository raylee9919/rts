/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Note: core functions.
//
internal void
ui_init(Ui_State *u)
{
    u->box_table_size   = 4096;
    u->box_table        = push_array(u->arena, Ui_Box_Slot, u->box_table_size);
}

internal b32
ui_key_match(Ui_Key a, Ui_Key b)
{
    return ( a.e[0] == b.e[0] );
}

internal u64
ui_slot_from_key(Ui_Key key)
{
    return ( key.e[0] % ui->box_table_size );
}

internal b32
ui_box_is_nil(Ui_Box *box)
{
    return ( box == NULL );
}

internal Ui_Key
ui_key_from_string(Utf8 string)
{
    Ui_Key seed = {};
    seed.e[0] = 5381;

    Ui_Key key = {};
    if (string.len > 0)
    {
        memory_copy(&key, &seed, sizeof(Ui_Key));
        for (u64 i = 0; i < string.len; ++i)
        {
            // # Todo: I'm just starting by being lazy and throwing djb2.
            //         https://theartincode.stanis.me/008-djb2/
            key.e[0] = ( (key.e[0] << 5) + key.e[0] ) + string.str[i];
        }
    }
    return key;
}

internal Ui_Box *
ui_box_from_key(Ui_Key key)
{
    Ui_Box *result = NULL;

    u64 slot = ui_slot_from_key(key);

    if (! ui_key_match(key, ui_key_zero))
    {
        for (Ui_Box *box = ui->box_table[slot].first;
             !ui_box_is_nil(box);
             box = box->hash_next)
        {
            if (ui_key_match(box->key, key))
            {
                result = box;
                break;
            }
        }
    }

    return result;
}

internal Ui_Signal
ui_signal_from_box(Ui_Box *box)
{
    Ui_Signal result = {};

    // # Note: eat events
    for (Os_Event *event = os->event_sentinel->next, *next = 0;
         event != os->event_sentinel;
         event = next)
    {
        next = event->next;

        if (box->flags & UI_BOX_FLAG_CLICKABLE)
        {
            if ((event->type == OS_EVENT_PRESS) &&
                (event->key == OS_KEY_MOUSE_LEFT))
            {
                if (1) // # Todo: box test
                {
                    os_event_consume(event);
                    result.clicked = 1;
                }
            }
        }
    }

    return result;
}

internal Ui_Box *
ui_build_box(Ui_Box_Flags flags, Ui_Key key)
{
    Ui_Box *result = push_struct(ui->arena, Ui_Box);
    {
        result->flags = flags;
    }

    u64 slot = ui_slot_from_key(key);
    sll_push_back_n(ui->box_table[slot].first, ui->box_table[slot].last, result, hash_next);

    return result;
}

global Ui_Box *current_box;


// # Note: builder functions.
//
#if 0
internal Ui_Signal
ui_button(Utf8 string)
{
    Ui_Key key = ui_key_from_string(string);
    Ui_Box *box = ui_box_from_key(key);

    if (ui_box_is_nil(box))
    {
        Ui_Box_Flags flags = (UI_BOX_FLAG_CLICKABLE | UI_BOX_FLAG_DRAW_BACKGROUND);
        box = ui_build_box(flags, key);
    }

    Ui_Signal result = ui_signal_from_box(box);
    return result;
}
#endif

internal Ui_Signal
ui_button(Utf8 string)
{
    Ui_Signal result = {};

    // # Size
    v2 origin = {2, 2};
    Ui_Size size = {};
    {
        size.type  = UI_SIZE_PX;
        size.value = {200, 100};
    }

    // # Signal
    for (Os_Event *event = os->event_sentinel->next, *next = NULL;
         event != os->event_sentinel;
         event = next)
    {
        next = event->next;

        if (event->type == OS_EVENT_PRESS && event->key == OS_KEY_MOUSE_LEFT)
        {
            f32 x = event->position.x;
            f32 y = event->position.y;
            if (x >= origin.x && x <= origin.x + size.value.x &&
                y >= origin.y && y <= origin.y + size.value.y)
            {
                os_event_consume(event);
                result.clicked = true;
            }
        }
    }

    // # Draw
    draw_quad(origin, origin + size.value);

    return result;
}
