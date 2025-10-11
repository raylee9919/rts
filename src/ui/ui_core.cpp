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
    {
        ui->build_arena[i] = arena_alloc(); 
    }

    ui->box_table_size = 4096;
    ui->box_table      = push_array(ui->permanent_arena, Ui_Box_Slot, ui->box_table_size);

    ui->base_family    = utf8lit("Segoe UI");
    ui->font_size      = 16.f;
}


// ----------------------------------------------------------------------------
// # Note: Frame Boundaries
//
internal void
ui_begin(f32 dt, u32 width, u32 height)
{
    // Receive delta time.
    ui_state->dt = dt;

    // Increment tick and clear the arena to be used on current frame.
    ui_state->tick += 1;
    arena_clear(ui_build_arena());


    // Clear style.
    ui_state->bg_first              = NULL;
    ui_state->hot_bg_first          = NULL;
    ui_state->flow_first            = NULL;
    ui_state->text_padding_first    = NULL;
    ui_state->corner_radius00_first = NULL;
    ui_state->corner_radius01_first = NULL;
    ui_state->corner_radius10_first = NULL;
    ui_state->corner_radius11_first = NULL;

    // Init style.
    ui_bg_push(v4{0.1f, 0.1f, 0.1f, 1.0f});
    ui_hot_bg_push(v4{0.18f, 0.18f, 0.18f, 1.0f});
    ui_text_padding_push(2.f);
    ui_style_push(flow, AXIS2_Y);
    ui_style_push(corner_radius00, 0.f);
    ui_style_push(corner_radius01, 0.f);
    ui_style_push(corner_radius10, 0.f);
    ui_style_push(corner_radius11, 0.f);
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, (f32)width);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, (f32)height);
    ui_seed_push(ui_key_zero);

    // Reset hot key.
    zero_struct(&ui_state->hot_key);
    zero_struct(&ui_state->active_key);


    // Build root.
    ui_state->root = ui_box_build_from_string(0, utf8lit("UI Root"));

    // Push root.
    ui_parent_push(ui_state->root);
}

internal void 
ui_end(void)
{
    assert(ui_state->current_parent == ui_state->root);

    for (Axis2 axis = AXIS2_X; axis < AXIS2_COUNT; ++axis)
    {
        ui_solve_size_independent(ui_state->root, axis);
        ui_solve_size_dependent_upward(ui_state->root, axis);
        ui_solve_size_dependent_downward(ui_state->root, axis);
        ui_solve_size_violation(ui_state->root, axis);
    }

    ui_animate();
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
    Ui_Box *box = ui_box_build_from_key(flags, key);

    box->debug_string = string;

    if (flags & UI_BOX_FLAG_DRAW_TEXT)
    {
        ui_equip_text(box, string);
    }

    return box;
}

internal Ui_Box *
ui_box_build_from_key(Ui_Box_Flags flags, Ui_Key key)
{
    Ui_Box *box = ui_box_from_key(key); 
    b32 first = 0;

    if (ui_box_is_nil(box))
    {
        box = ui_box_alloc();
        u64 slot = key.e[0] % ui_state->box_table_size;
        sll_push_back_n(ui_state->box_table[slot].first, ui_state->box_table[slot].last, box, hash_next);
        first = 1;
    }

    if (ui_state->current_parent)
    {
        sll_push_back(ui_state->current_parent->first, ui_state->current_parent->last, box);
    }

    // Initialize state.
    box->key            = key;
    box->parent         = ui_state->current_parent;
    box->flags          = flags;
    box->flow           = ui_style_top(flow);
    box->touched_tick   = ui_state->tick;
    box->first_tick     = first ? ui_state->tick : box->first_tick;

    box->bg              = ui_bg_top();
    box->hot_bg          = ui_hot_bg_top();
    box->corner_radius00 = ui_style_top(corner_radius00);
    box->corner_radius01 = ui_style_top(corner_radius01);
    box->corner_radius10 = ui_style_top(corner_radius10);
    box->corner_radius11 = ui_style_top(corner_radius11);

    box->semantic_size[AXIS2_X] = ui_size_top(AXIS2_X);
    box->semantic_size[AXIS2_Y] = ui_size_top(AXIS2_Y);

    // Push seed
    ui_seed_push(key);

    return box;
}

internal void
ui_equip_text(Ui_Box *box, Utf8 text)
{
    box->text = push_struct(ui_build_arena(), Ui_Text);
    {
        Fp_Draw_String_Result dsr = fp_draw_string(text, ui_state->base_family, ui_state->font_size, V2(0.f), RENDER_STRING_FLAG_NO_DRAW|RENDER_STRING_FLAG_COMPUTE_SIZE);

        box->text->string      = text;
        box->text->aabb        = dsr.aabb;
        box->text->max_ascent  = dsr.max_ascent;
        box->text->max_descent = dsr.max_descent;
        box->text->padding     = ui_text_padding_top();
    }

    if (ui_style_top(flow) == AXIS2_X)
    {
        box->semantic_size[AXIS2_X].type = UI_SIZE_TYPE_TEXT;
    }
    else
    {
        box->semantic_size[AXIS2_X].type  = UI_SIZE_TYPE_PCT;
        box->semantic_size[AXIS2_X].value = 1.0f;
    }
    box->semantic_size[AXIS2_Y].type = UI_SIZE_TYPE_TEXT;
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
                    root->computed_size[axis] = root->text->aabb.max.x - root->text->aabb.min.x + 2.f*root->text->padding;
                }break;

                case AXIS2_Y: {
                    root->computed_size[axis] = root->text->max_ascent + root->text->max_descent + 2.f*root->text->padding;
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

            if (root->flow == axis)
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
ui_draw(Ui_Box *root, v2 root_position)
{
    v2 min = root_position + v2{root->relative_position[AXIS2_X], root->relative_position[AXIS2_Y]};
    v2 max = min + v2{root->computed_size[AXIS2_X], root->computed_size[AXIS2_Y]};

    // Draw contents background.
    if (root->flags & UI_BOX_FLAG_DRAW_BACKGROUND)
    {
        if (root->flags & UI_BOX_FLAG_DRAW_SHOOT_EFFECT && root->shoot_t > 0.001f)
        {
            v4 c = v4{0.5f,0.5f,1.0f,1.0f};
            c = lerp(c, root->shoot_t, root->hot_bg);
            render_quad_c4r4(min, max, root->hot_bg,root->hot_bg,root->hot_bg,root->hot_bg, root->corner_radius00, root->corner_radius01, root->corner_radius10, root->corner_radius11);
            render_quad_c4r4(min, max, c,c,c,c, root->corner_radius00, root->corner_radius01, root->corner_radius10, root->corner_radius11);
        }
        else if (root->flags & UI_BOX_FLAG_DRAW_ACTIVE_EFFECT && root->active_t > 0.001f)
        {
            v4 c = v4{0.5f,0.5f,1.0f,1.0f};
            c.rgb *= root->active_t;
            render_quad_c4r4(min, max, c,c,c,c, root->corner_radius00, root->corner_radius01, root->corner_radius10, root->corner_radius11);
        }
        else if (root->flags & UI_BOX_FLAG_DRAW_HOT_EFFECT && root->hot_t > 0.001f)
        {
            v4 c = lerp(root->bg, root->hot_t, root->hot_bg);
            render_quad_c4r4(min, max, c,c,c,c, root->corner_radius00, root->corner_radius01, root->corner_radius10, root->corner_radius11);
        }
        else
        {
            render_quad_c4r4(min, max, root->bg,root->bg,root->bg,root->bg, root->corner_radius00, root->corner_radius01, root->corner_radius10, root->corner_radius11);
        }
    }


    // Draw text.
    if (root->flags & UI_BOX_FLAG_DRAW_TEXT)
    {
        assert(root->text != NULL);

        v2 pen = min + V2(root->text->padding) + v2{-root->text->aabb.min.x, root->text->max_ascent};
        AABB2 cull_aabb = {};
        cull_aabb.min = min;
        cull_aabb.max = max;

        fp_draw_string(root->text->string, ui_state->base_family, ui_state->font_size, pen, RENDER_STRING_FLAG_CULL, cull_aabb);
    }

    // Recursive traversal
    for (Ui_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
        ui_draw(child, min);
    }
}

internal void
ui_animate(void)
{
    f32 rate = 1 - pow(2.f, -50.f * ui_state->dt*0.2f);

    for (u64 slot = 0; slot < ui_state->box_table_size; ++slot)
    {
        for (Ui_Box *box = ui_state->box_table[slot].first; !ui_box_is_nil(box); box = box->hash_next)
        {
            b32 is_hot    = ui_key_match(ui_state->hot_key, box->key);
            b32 is_active = ui_key_match(ui_state->active_key, box->key);

            box->hot_t    += ((f32)!!is_hot    - box->hot_t)   *rate;
            box->active_t += ((f32)!!is_active - box->active_t)*rate;

            box->shoot_t = max(box->shoot_t - rate, 0.f);
        }
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
        for (Os_Event *event = os->event_sentinel->next, *next; event != os->event_sentinel; event = next)
        {
            next = event->next;

            if (box->flags & UI_BOX_FLAG_MOUSE_CLICKABLE)
            {
                if (event->type == OS_EVENT_PRESS && event->key == OS_KEY_MOUSE_LEFT)
                {
                    if (intersects(aabb, event->position))
                    {
                        os_event_consume(event);
                        result.pressed_left = true;

                        if (box->flags & UI_BOX_FLAG_DRAW_SHOOT_EFFECT)
                        {
                            box->shoot_t = 1.0f;
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
    Ui_Key seed = ui_seed_top();

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
    return (box == NULL || box == &ui_nil_box);
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

internal void
ui_seed_push(Ui_Key seed)
{
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);
    style->seed = seed;

    stack_push(ui_state->seed_first, style);
}

internal void
ui_seed_pop(void)
{
    stack_pop(ui_state->seed_first);
}

internal Ui_Key
ui_seed_top(void)
{
    return ui_state->seed_first->seed;
}


internal void
ui_bg_push(v4 color)
{
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);
    style->bg = color;

    stack_push(ui_state->bg_first, style);
}

internal void
ui_bg_pop(void)
{
    stack_pop(ui_state->bg_first);
}

internal v4
ui_bg_top(void)
{
    return ui_state->bg_first->bg;
}


internal void
ui_hot_bg_push(v4 color)
{
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);
    style->hot_bg = color;

    stack_push(ui_state->hot_bg_first, style);
}

internal void
ui_hot_bg_pop(void)
{
    stack_pop(ui_state->hot_bg_first);
}

internal v4
ui_hot_bg_top(void)
{
    return ui_state->hot_bg_first->hot_bg;
}


internal void
ui_text_padding_push(f32 padding)
{
    Ui_Style *style = push_struct(ui_build_arena(), Ui_Style);
    style->text_padding = padding;

    stack_push(ui_state->text_padding_first, style);
}

internal void
ui_text_padding_pop(void)
{
    stack_pop(ui_state->text_padding_first);
}

internal f32
ui_text_padding_top(void)
{
    return ui_state->text_padding_first->text_padding;
}
