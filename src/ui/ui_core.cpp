/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// ----------------------------------------------------------------------------
// # Note: Alloc/Init
//
internal Ui_State *
ui_alloc(void)
{
    Arena *arena = arena_alloc();
    Ui_State *result = push_struct(arena, Ui_State);
    result->permanent_arena = arena;
    return result;
}

internal void
ui_init(Ui_State *ui)
{
    for (u32 i = 0; i < array_count(ui->build_arena); ++i)
    { ui->build_arena[i] = arena_alloc(); }

    ui->box_table_size = 4096;
    ui->box_table      = push_array(ui->permanent_arena, Ui_Box_Slot, ui->box_table_size);
}


// ----------------------------------------------------------------------------
// # Note: Frame Boundaries
//
internal void
ui_begin(f32 dt)
{
    ui_state->dt = dt;

    ui_state->tick += 1;
    arena_clear(ui_build_arena());

    // Clear style.
    ui_state->bg_first              = NULL;
    ui_state->flow_first            = NULL;
    ui_state->text_padding_first    = NULL;
    ui_state->corner_radius00_first = NULL;
    ui_state->corner_radius01_first = NULL;
    ui_state->corner_radius10_first = NULL;
    ui_state->corner_radius11_first = NULL;

    // Init style.
    ui_style_push(bg, V4(0.1f, 0.1f, 0.1f, 1.0f));
    ui_style_push(flow, AXIS2_Y);
    ui_style_push(text_padding, 2.f);
    ui_style_push(corner_radius00, 0.f);
    ui_style_push(corner_radius01, 0.f);
    ui_style_push(corner_radius10, 0.f);
    ui_style_push(corner_radius11, 0.f);
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, 1920.f);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 1080.f);

    // Reset hot key.
    zero_struct(&ui_state->hot_key);


    // Build root.
    ui_state->root = ui_box_build_from_string(0, utf8lit("UI Root"));

    // Push root.
    ui_parent_push(ui_state->root);
}

internal void 
ui_end(void)
{
    for (Axis2 axis = AXIS2_X; axis < AXIS2_COUNT; ++axis)
    {
        ui_solve_size_independent(ui_state->root, axis);
        ui_solve_size_dependent_upward(ui_state->root, axis);
        ui_solve_size_dependent_downward(ui_state->root, axis);
        ui_solve_size_violation(ui_state->root, axis);
    }

    ui_draw(ui_state->root, v2{});
}

// ----------------------------------------------------------------------------
// # Note: Stack
//
internal void
ui_parent_push(Ui_Box *box)
{
    ui_state->current_parent = box;
}

internal void
ui_parent_pop(void)
{
    ui_state->current_parent = ui_state->current_parent->parent;
}

// ----------------------------------------------------------------------------
// # Note: Box Build
//
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
        box = push_struct(ui_state->permanent_arena, Ui_Box);
    }

    return box;
}

internal Ui_Box *
ui_box_build_from_string(Ui_Box_Flags flags, Utf8 string)
{
    Ui_Key key  = ui_key_from_string(string);
    return ui_box_build_from_key(flags, key);
}

internal Ui_Box *
ui_box_build_from_key(Ui_Box_Flags flags, Ui_Key key)
{
    Ui_Box *box = ui_box_from_key(key); 

    if (ui_box_is_nil(box))
    {
        box = ui_box_alloc();
        u64 slot = key.e[0] % ui_state->box_table_size;
        sll_push_back_n(ui_state->box_table[slot].first, ui_state->box_table[slot].last, box, hash_next);
    }

    if (ui_state->current_parent)
    {
        sll_push_back(ui_state->current_parent->first, ui_state->current_parent->last, box);
    }

    // Initialize state.
    box->key    = key;
    box->parent = ui_state->current_parent;
    box->flags  = flags;
    box->flow   = ui_style_top(flow);

    box->bg = ui_style_top(bg);
    box->corner_radius00 = ui_style_top(corner_radius00);
    box->corner_radius01 = ui_style_top(corner_radius01);
    box->corner_radius10 = ui_style_top(corner_radius10);
    box->corner_radius11 = ui_style_top(corner_radius11);

    box->semantic_size[AXIS2_X] = ui_size_top(AXIS2_X);
    box->semantic_size[AXIS2_Y] = ui_size_top(AXIS2_Y);

    return box;
}

internal void
ui_equip_text(Ui_Box *box, Utf8 text)
{
    box->text = push_struct(ui_build_arena(), Ui_Text);
    {
        box->text->string  = text;
        box->text->aabb    = render_string(ui_state->face, ui_state->texture_id, v2{}, text, RENDER_STRING_FLAG_NO_DRAW|RENDER_STRING_FLAG_COMPUTE_SIZE);
        box->text->padding = ui_style_top(text_padding);
    }
}

// ----------------------------------------------------------------------------
// @Note: Solve Size.
//
internal void
ui_solve_size_independent(Ui_Box *root, Axis2 axis)
{
    switch (root->semantic_size[axis].type)
    {
        case UI_SIZE_TYPE_PX:
        {
            root->computed_size[axis] = root->semantic_size[axis].value;
        }break;

        case UI_SIZE_TYPE_TEXT:
        {
            switch (axis)
            {
                case AXIS2_X: {
                    root->computed_size[axis] = root->text->aabb.max.x - root->text->aabb.min.x + 2.f*ui_style_top(text_padding);
                }break;

                case AXIS2_Y: {
                    root->computed_size[axis] = ui_state->face->ascent + ui_state->face->descent + 2.f*ui_style_top(text_padding);
                }break;

                default: { assert(! "Invalid Axis."); }break;
            }
        }break;

        default: break;
    }

    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        ui_solve_size_independent(child, axis);
    }
}

internal void
ui_solve_size_dependent_upward(Ui_Box *root, Axis2 axis)
{
    switch (root->semantic_size[axis].type)
    {
        case UI_SIZE_TYPE_PCT: 
        {
            Ui_Box *ancestor = NULL;
            for (Ui_Box *p = root->parent; !ui_box_is_nil(p); p = p->parent)
            {
                if (p->semantic_size[axis].type != UI_SIZE_TYPE_CHILDREN)
                {
                    ancestor = p;
                    break;
                }
            }

            if (! ui_box_is_nil(ancestor))
            {
                root->computed_size[axis] = ancestor->computed_size[axis] * root->semantic_size[axis].value;
            }
        }break;

        default: break;
    }

    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        ui_solve_size_dependent_upward(child, axis);
    }
}

internal void
ui_solve_size_dependent_downward(Ui_Box *root, Axis2 axis)
{
    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        ui_solve_size_dependent_downward(child, axis);
    }

    switch (root->semantic_size[axis].type)
    {
        case UI_SIZE_TYPE_CHILDREN: 
        {
            f32 value = 0.f;

            if (ui_style_top(flow) == axis)
            {
                for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
                {
                    value += child->computed_size[axis];
                }
            }
            else
            {
                for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
                {
                    value = max(value, child->computed_size[axis]);
                }
            }

            root->computed_size[axis] = value;
        }break;

        default: break;
    }
}

internal void
ui_solve_size_violation(Ui_Box *root, Axis2 axis)
{
    f32 budget = root->computed_size[axis];

    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        if (root->flow == axis)
        {
            if (budget >= child->computed_size[axis])
            {
                budget -= child->computed_size[axis];
            }
            else if (budget > 0.f)
            {
                child->computed_size[axis] -= budget;
                budget = 0.f;
            }
            else
            {
                child->computed_size[axis] = 0.f;
            }
        }
    }

    // Solve position.
    f32 p = 0.f;
    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        if (root->flow == axis)
        {
            child->relative_position[axis] = p;
            p += child->computed_size[axis];
        }
        else
        {
            child->relative_position[axis] = 0.f;
        }

        child->position[axis] = root->position[axis] + child->relative_position[axis];
    }

    // Recursive traversal.
    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        ui_solve_size_violation(child, axis);
    }
}

// ----------------------------------------------------------------------------
// # Note: Draw
//
internal void
ui_draw(Ui_Box *child, v2 root_position)
{
    v2 min = root_position + v2{child->relative_position[AXIS2_X], child->relative_position[AXIS2_Y]};
    v2 max = min + v2{child->computed_size[AXIS2_X], child->computed_size[AXIS2_Y]};

    // Draw contents background.
    if (child->flags & UI_BOX_FLAG_DRAW_BACKGROUND)
    {
        if (child->flags & UI_BOX_FLAG_DRAW_HOT_EFFECT && ui_key_match(ui_state->hot_key, child->key))
        {
            v4 c = child->bg;
            c.rgb *= 1.3f;
            render_quad_c4r4(min, max, c,c,c,c, child->corner_radius00, child->corner_radius01, child->corner_radius10, child->corner_radius11);
        }
        else
        {
            render_quad_c4r4(min, max, child->bg,child->bg,child->bg,child->bg, child->corner_radius00, child->corner_radius01, child->corner_radius10, child->corner_radius11);
        }
    }

    // Draw text.
    if (child->flags & UI_BOX_FLAG_DRAW_TEXT)
    {
        assert(child->text != NULL);

        v2 pen = min + V2(child->text->padding) + v2{child->text->aabb.min.x, ui_state->face->ascent};
        AABB2 cull_aabb = {};
        cull_aabb.min = min;
        cull_aabb.max = max;

        render_string(ui_state->face, ui_state->texture_id, pen, child->text->string, RENDER_STRING_FLAG_CULL, cull_aabb);
    }

    // Recursive traversal
    for (Ui_Box *child2 = child->first; !ui_box_is_nil(child2); child2 = child2->next)
    {
        ui_draw(child2, min);
    }
}


// ----------------------------------------------------------------------------
// # Note: Signal
//
internal Ui_Signal
ui_signal_from_box(Ui_Box *box)
{
    Ui_Signal result = {};

    AABB2 aabb = {};
    aabb.min = v2{box->position[AXIS2_X], box->position[AXIS2_Y]};
    aabb.max = aabb.min + v2{box->computed_size[AXIS2_X], box->computed_size[AXIS2_Y]};

    if (! ui_box_is_nil(box))
    {
        if (box->flags & UI_BOX_FLAG_MOUSE_CLICKABLE || box->flags & UI_BOX_FLAG_KEYBOARD_CLICKABLE)
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
                        if (intersects(aabb, event->position))
                        {
                            os_event_consume(event);
                            result.pressed_left = true;
                        }
                    }
                }
            }
        }
    }


    if (intersects(aabb, os->mouse_position_last))
    {
        ui_state->hot_key = box->key;
    }

    return result;
}





// ----------------------------------------------------------------------------
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

internal Arena *
ui_build_arena(void)
{
    u64 index = ui_state->tick % array_count(ui_state->build_arena);
    return ui_state->build_arena[index];
}




// @Note: Push style state stack frames.
//
internal void
ui_size_push(Axis2 axis, Ui_Size_Type type, f32 value = 0.f)
{
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);
    style->size.type = type;
    style->size.value = value;

    stack_push(ui_state->size_first[axis], style);
}

internal void
ui_size_pop(Axis2 axis)
{
    stack_pop(ui_state->size_first[axis]);
}

internal Ui_Size
ui_size_top(Axis2 axis)
{
    return ui_state->size_first[axis]->size;
}
