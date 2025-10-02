/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Note: Alloc/Init
//
internal Ui_State *
ui_alloc(void)
{
    Arena *arena = arena_alloc();
    Ui_State *result = push_struct(arena, Ui_State);
    result->arena = arena;
    return result;
}

internal void
ui_init(Ui_State *ui)
{
    ui->box_arena      = arena_alloc();
    ui->box_table_size = 4096;
    ui->box_table      = push_array(ui->arena, Ui_Box_Slot, ui->box_table_size);

    // # Note: Alloc root node.
    //
    ui->current_parent = ui->root = push_struct(ui->box_arena, Ui_Box);
    {
        Ui_Box *root = ui->root;
        root->flags = UI_BOX_FLAG_FLOW_X;
        root->semantic_size[0].type = UI_SIZE_TYPE_CHILDREN;
        root->semantic_size[1].type = UI_SIZE_TYPE_CHILDREN;

        root->padding = 4.f;
        root->border  = 2.f;
        root->margin  = 0.f;

        root->bg[0] = v4{0.2f, 0.2f, 0.2f, 0.9f};
        root->bg[1] = v4{0.2f, 0.2f, 0.2f, 0.9f};
        root->bg[2] = v4{0.0f, 0.0f, 0.0f, 0.9f};
        root->bg[3] = v4{0.0f, 0.0f, 0.0f, 0.9f};
    }
}

// # Note: Helper Functions.
//
internal b32
ui_key_match(Ui_Key a, Ui_Key b)
{
    return ( a.e[0] == b.e[0] );
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

internal b32
ui_box_is_nil(Ui_Box *box)
{
    return (box == NULL);
}

internal Ui_Box *
ui_box_from_key(Ui_Key key)
{
    Ui_Box *result = NULL;

    u64 slot = (key.e[0] % ui_state->box_table_size);

    if (! ui_key_match(key, ui_key_zero))
    {
        for (Ui_Box *box = ui_state->box_table[slot].first;
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

internal Ui_Box *
ui_box_alloc(void)
{
    Ui_Box *box = ui_state->first_free_box;

    if (box != NULL)
    {
        zero_memory(box, sizeof(*box));
        sll_pop_front(ui_state->first_free_box, ui_state->last_free_box);
    }
    else
    {
        box = push_struct(ui_state->box_arena, Ui_Box);
    }

    return box;
}

internal Ui_Box *
ui_push(Utf8 string, Ui_Box_Flags flags)
{
    Ui_Box *parent = ui_state->current_parent;

    Ui_Key key = ui_key_from_string(string);
    Ui_Box *box = ui_box_from_key(key);

    if (ui_box_is_nil(box))
    {
        box = ui_box_alloc();
        {
            // # Note: Init box
            box->key = key;
            box->parent = parent;
            box->flags = flags;
            box->text = string;
        }

        u64 slot = (key.e[0] % ui_state->box_table_size);
        sll_push_back_n(ui_state->box_table[slot].first, ui_state->box_table[slot].last, box, hash_next);

        if (! ui_box_is_nil(parent))
        {
            sll_push_back(parent->first, parent->last, box);

            // # Note: Inherit
            box->padding = parent->padding;
            box->border  = parent->border;
            box->margin  = parent->margin;
            memory_copy(box->bg, parent->bg, sizeof(box->bg[0]) * array_count(box->bg));
        }
    }

    ui_state->current_parent = box;

    return box;
}

internal void
ui_pop(void)
{
    if (ui_state->current_parent)
    {
        ui_state->current_parent = ui_state->current_parent->parent;
    }
    else
    {
        assert(! "Current parent is null.");
    }
}


// # Note: Compute size
//
internal v2 
ui_compute_size(Ui_Box *box)
{
    // # Note: Depth first post order traverse.
    //

    v2 result = {};

    for (Ui_Box *child = box->first; child != NULL; child = child->next)
    {
        v2 child_size = ui_compute_size(child);

        if (box->semantic_size[0].type == UI_SIZE_TYPE_CHILDREN)
        {
            if (box->flags & UI_BOX_FLAG_FLOW_X)
            {
                result.x += child_size.x;
            }
            else if (box->flags & UI_BOX_FLAG_FLOW_Y)
            {
                result.x = max(result.x, child_size.x);
            }
            else
            {
                assert(! "Unkown flow.");
            }
        }

        if (box->semantic_size[1].type == UI_SIZE_TYPE_CHILDREN)
        {
            if (box->flags & UI_BOX_FLAG_FLOW_Y)
            {
                result.y += child_size.y;
            }
            else if (box->flags & UI_BOX_FLAG_FLOW_X)
            {
                result.y = max(result.y, child_size.y);
            }
            else
            {
                assert(! "Unknown flow.");
            }
        }
    }


    for (u32 axis = 0; axis < 2; ++axis)
    {
        if (box->semantic_size[axis].type != UI_SIZE_TYPE_CHILDREN)
        {
            if (box->semantic_size[axis].type == UI_SIZE_TYPE_PX)
            {
                result.e[axis] = box->semantic_size[axis].value;
            }
            else if (box->semantic_size[axis].type == UI_SIZE_TYPE_TEXT)
            {
                // # Fix: Redundant.
                AABB2 text_aabb = render_string(ui_state->face, ui_state->texture_id, v2{}, box->text, RENDER_STRING_FLAG_NO_DRAW|RENDER_STRING_FLAG_COMPUTE_SIZE);
                box->text_aabb = text_aabb;
                if (axis == 0)
                {
                    result.e[axis] = text_aabb.max.e[axis] - text_aabb.min.e[axis];
                }
                else
                {
                    result.e[axis] = ui_state->face->ascent + ui_state->face->descent;
                }
            }
            else
            {
                assert(! "Unknown ui size type.");
            }
        }
    }

    result += V2( 2.f * (box->padding + box->border) );
    box->computed_size = result;

    return result;
}


// # Note: Draw
//
internal void
ui_box_draw(Ui_Box *box)
{
    if (box != ui_state->root)
    {
        v2 min = box->position;
        v2 max = box->position + box->computed_size;

        // # Note: Draw border.
        //

        if (box->border != 0.f)
        {
            render_quad_c(min, max, V4(0.0f));
            min += V2(box->border);
            max -= V2(box->border);
        }
        

        // # Note: Draw contents background.
        //
        if (box->flags & UI_BOX_FLAG_DRAW_BACKGROUND)
        {
            f32 radius = min(box->computed_size.x, box->computed_size.y) * 0.16f;
            render_quad_c4r(min, max, box->bg[0], box->bg[1], box->bg[2], box->bg[3], radius);
        }
        min += V2(box->padding);
        max -= V2(box->padding);

        // # Note: Draw text.
        //
        if (box->flags & UI_BOX_FLAG_DRAW_TEXT && box->text.str != NULL)
        {
            f32 left = box->text_aabb.min.x;
            f32 top = ui_state->face->ascent;
            v2 pen = min + v2{left, top};
            if (box->flags & UI_BOX_FLAG_TEXT_ALIGN_CENTER)
            {
                f32 a = (box->computed_size.x - 2.f*(box->border + box->padding));
                f32 b = (box->text_aabb.max.x - box->text_aabb.min.x);
                f32 x = 0.5f * (a - b);
                pen.x += x;
                render_string(ui_state->face, ui_state->texture_id, pen, box->text, 0);
            }
            else
            {
                render_string(ui_state->face, ui_state->texture_id, pen, box->text, 0);
            }
        }
    }
}


// # Note: Compute Position
//
internal void 
ui_compute_position(Ui_Box *box, v2 position)
{
    // # Note: Depth first pre order traversal.
    //
    box->position = position;
    v2 pen = position;

    // # Note: Broadcast to it's children about it's size so they can span.
    //
    for (Ui_Box *child = box->first; child != NULL; child = child->next)
    {
        if (box->flags & UI_BOX_FLAG_FLOW_Y)
        {
            if (child->semantic_size[0].type == UI_SIZE_TYPE_TEXT)
            {
                child->computed_size.x = box->computed_size.x - 2.f*(box->border + box->padding);
            }
            else if (child->semantic_size[0].type == UI_SIZE_TYPE_CHILDREN)
            {
                child->computed_size.x = box->computed_size.x - 2.f*(box->border + box->padding);
            }
            else
            {
                assert(! "Invalid size type.");
            }
        }
    }


    // # Note: You can draw in the same pass!
    ui_box_draw(box);
    pen += V2(box->border + box->padding);


    for (Ui_Box *child = box->first; child != NULL; child = child->next)
    {
        ui_compute_position(child, pen);

        if (box->flags & UI_BOX_FLAG_FLOW_X)
        {
            pen.x += child->computed_size.x;
        }
        else if (box->flags & UI_BOX_FLAG_FLOW_Y)
        {
            pen.y += child->computed_size.y;
        }
        else
        {
            assert(! "Unknown flow.");
        }
    }
}



// # Note: Signal
//
internal Ui_Signal
ui_signal_from_box(Ui_Box *box)
{
    Ui_Signal result = {};

    if (box != NULL)
    {
        if (box->flags & UI_BOX_FLAG_MOUSE_CLICKABLE ||
            box->flags & UI_BOX_FLAG_KEYBOARD_CLICKABLE)
        {
            for (Os_Event *event = os->event_sentinel->next, *next;
                 event != os->event_sentinel;
                 event = next)
            {
                next = event->next;

                if (event->type == OS_EVENT_PRESS)
                {
                    if (event->key == OS_KEY_MOUSE_LEFT)
                    {
                        if (event->position.x >= box->position.x  &&  event->position.x <= box->position.x + box->computed_size.x &&
                            event->position.y >= box->position.y  &&  event->position.y <= box->position.y + box->computed_size.y)
                        {
                            os_event_consume(event);
                            result.pressed_left = true;
                        }
                    }
                }
            }
        }
    }

    return result;
}
