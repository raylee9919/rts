// Copyright Seong Woo Lee. All Rights Reserved.



#define READ(to, type)\
    memcpy(&to, at, sizeof(type)); \
    at += sizeof(to);
#define READ_COUNT(to, type, count) \
    to = push_array(arena, type, count); \
    memcpy(to, at, sizeof(type)*count); \
    at += (sizeof(type)*count);



internal void
asset_load_model(Model *model, Utf8 file_path, Arena *arena, v3 scale)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    assert(model);

    // Note: read entire file.
    //
    Utf8 entire_file = read_entire_file(scratch.arena, file_path);
    u8 *at  = entire_file.str;
    u8 *end = at + entire_file.len;

    READ(model->mesh_count, u32);

    model->meshes = push_array(arena, Mesh, model->mesh_count);
    for (u32 mesh_idx = 0; mesh_idx < model->mesh_count; ++mesh_idx)
    {
        Mesh *mesh = model->meshes + mesh_idx;

        READ(mesh->vertex_count, u32);
        mesh->vertices = push_array(arena, Vertex, mesh->vertex_count);
        for (u32 vertex_idx = 0; vertex_idx < mesh->vertex_count; ++vertex_idx)
        {
            Vertex *vertex = mesh->vertices + vertex_idx;
            READ(vertex->position, v3);
            vertex->position = hadamard(vertex->position, scale);
            READ(vertex->normal, v3);
            READ(vertex->uv, v2);
            READ(vertex->color, v4);
            READ(vertex->tangent, v3);

            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                READ(vertex->node_ids[i], s32); 
            }
            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                READ(vertex->node_weights[i], f32); 
            }
        }

        READ(mesh->index_count, u32);
        READ_COUNT(mesh->indices, u32, mesh->index_count);
    }

    // Material
    //
    READ(model->material_count, u32);
    READ_COUNT(model->materials, Material, model->material_count);

    // Nodes
    //
    READ(model->node_count, u32);
    if (model->node_count)
    {
        READ(model->root_bone_node_id, s32);
        model->nodes = push_array(arena, Node, model->node_count);

        for (u32 node_idx = 0; node_idx < model->node_count; ++node_idx)
        {
            Node* node = model->nodes + node_idx;

            READ(node->id, s32);
            READ(node->offset, m4x4);
            READ(node->base_transform, m4x4);
            READ(node->child_count, u32);
            READ_COUNT(node->child_ids, s32, node->child_count);
        }
    }

    assert(at == end);
}

internal u32
get_triangle_count(Model *model)
{
    u32 result = 0;
    for (u32 i = 0; i < model->mesh_count; ++i) {
        Mesh *mesh = model->meshes + i;
        result += mesh->index_count;
    }
    result /= 3;
    return result;
}

internal u64
animation_hash(u32 id, u32 length) {
    u64 x = ( ((u64)id << 32) | (u64)length );
    u64 slot = XXH3_64bits_withSeed(&x, sizeof(x), 0) % length;
    return slot;
}

internal void
asset_load_animation(Animation* anim, Utf8 file_path, Arena *arena)
{
    assert(anim);

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);

    u8* at  = entire_file.str;
    u8* end = at + entire_file.len;

    READ_COUNT(anim->name, char, string_length((char *)at) + 1);

    READ(anim->duration, f32);
    READ(anim->sample_count, u32);

    anim->samples = push_array(arena, Sample, anim->sample_count);
    for (u32 sample_idx = 0;
         sample_idx < anim->sample_count;
         ++sample_idx)
    {
        Sample *sample = anim->samples + sample_idx;

        READ(sample->id, s32);

        READ(sample->translation_count, u32);
        READ(sample->rotation_count, u32);
        READ(sample->num_scales, u32);

        READ_COUNT(sample->translations, dt_v3_Pair, sample->translation_count);
        READ_COUNT(sample->rotations, dt_qt_Pair, sample->rotation_count);
        READ_COUNT(sample->scales, dt_v3_Pair, sample->num_scales);
    }

    assert(at == end);

    //
    // Build hash-table (key: node_id, value: node_idx in Animation->nodes)
    //
    Animation_Hash_Table *ht = &anim->hash_table;
    ht->entry_count = anim->sample_count;
    ht->entries = push_array(arena, Animation_Hash_Entry, ht->entry_count);
    for (u32 sample_idx = 0;
         sample_idx < anim->sample_count;
         ++sample_idx)
    {
        Sample *sample = anim->samples + sample_idx;
        u64 entry_idx = animation_hash(sample->id, ht->entry_count);
        Animation_Hash_Entry *entry = ht->entries + entry_idx;
        Animation_Hash_Slot *slot = entry->first;
        if (slot)
        {
            while (slot->next)
            {
                slot = slot->next;
            }
            slot->next = push_struct(arena, Animation_Hash_Slot);
            Animation_Hash_Slot *new_slot = slot->next;
            new_slot->id = sample->id;
            new_slot->idx = sample_idx;
            new_slot->next = 0;
        } else {
            slot = push_struct(arena, Animation_Hash_Slot);
            slot->id = sample->id;
            slot->idx = sample_idx;
            slot->next = 0;

            entry->first = slot;
        }
    }
}

internal void
asset_load_image(Bitmap *bitmap, Utf8 file_path, Arena *arena)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    assert(bitmap);

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);

    u8 *at  = entire_file.str;
    u8 *end = at + entire_file.len;

    READ(bitmap->bits_per_channel, s32);
    READ(bitmap->channel_count, s32);
    READ(bitmap->width, s32);
    READ(bitmap->height, s32);
    READ(bitmap->pitch, s32);
    bitmap->handle = 0;
    READ(bitmap->size, u32);
    bitmap->memory = push_array(arena, u8, bitmap->size); // @Memory!
    READ_COUNT(bitmap->memory, u8, bitmap->size);

    assert(at == end);
}

internal void
asset_load_image_general_format(Bitmap *bitmap, Utf8 file_path, Arena *arena)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    assert(bitmap);

    Utf8 contents = read_entire_file(scratch.arena, file_path);

    stbi_set_flip_vertically_on_load(true);
    int x, y, num_channels;
    u8 *data = stbi_load_from_memory(contents.str, (int)contents.len, &x, &y, &num_channels, 0);

    bitmap->bits_per_channel = 8;
    bitmap->channel_count    = num_channels;
    bitmap->width            = x;
    bitmap->height           = y;
    bitmap->pitch            = (bitmap->width*bitmap->bits_per_channel*bitmap->channel_count)>>3;
    bitmap->size             = (bitmap->width*bitmap->height*bitmap->bits_per_channel*bitmap->channel_count);
    bitmap->handle           = 0;
    bitmap->memory           = data;

    // @Todo: release memory via cstd api.
}

#undef READ
#undef READ_COUNT

// Note: Animation
//

internal Node_Hash_Result
get_sample_index(Animation *anim, u32 id) 
{
    Node_Hash_Result result = {};

    Animation_Hash_Table *ht = &anim->hash_table;
    u64 entry_idx = animation_hash(id, ht->entry_count);
    Animation_Hash_Entry *entry = ht->entries + entry_idx;
    for (Animation_Hash_Slot *slot = entry->first;
         slot;
         slot = slot->next)
    {
        if (slot->id == id)
        {
            result.found = true;
            result.idx = slot->idx;
            break;
        }
    }

    return result;
}

internal void
anim_accumulate(Animation_Channel *channel, f32 dt)
{
    if (channel->animation) 
    {
        channel->dt += dt;
        if (channel->dt > channel->animation->duration) 
        {
            channel->dt = 0.0f;
        }
    }
}

internal Xform
interpolate_trs(Xform trs1, f32 t, Xform trs2)
{
    Xform result = {};
    result.translation = lerp(trs1.translation, t, trs2.translation);

    Quaternion first  = trs1.rotation;
    Quaternion second = trs2.rotation;
    if (dot(first, second) < 0.f) {
        second = -second;
    }
    result.rotation = nlerp(first, t, second);

    result.scale = lerp(trs1.scale, t, trs2.scale);
    return result;
}

internal Xform
interpolate_sample(Sample* sample, f32 dt)
{
    Xform result = {};

    // Translation
    result.translation = (sample->translations + (sample->translation_count - 1))->vec;
    for (u32 translation_idx = 0; translation_idx < sample->translation_count; ++translation_idx)
    {
        dt_v3_Pair *hi_key = sample->translations + translation_idx;
        if (hi_key->dt > dt) {
            dt_v3_Pair *lo_key = (hi_key - 1);
            f32 t = (dt - lo_key->dt) / (hi_key->dt - lo_key->dt);
            result.translation = lerp(lo_key->vec, t, hi_key->vec);
            break;
        } else if (hi_key->dt == dt) {
            result.translation = hi_key->vec;
            break;
        }
    }

    // Rotation
    result.rotation = (sample->rotations + (sample->rotation_count - 1))->q;
    for (u32 rotation_idx = 0; rotation_idx < sample->rotation_count; ++rotation_idx) 
    {
        dt_qt_Pair* hi_key = sample->rotations + rotation_idx;
        if (hi_key->dt > dt) {
            dt_qt_Pair* lo_key = (hi_key - 1);
            f32 t = (dt - lo_key->dt) / (hi_key->dt - lo_key->dt);
            Quaternion first  = lo_key->q;
            Quaternion second = hi_key->q;
            if (dot(first, second) < 0.f) {
                second = -second;
            }
            result.rotation = nlerp(first, t, second);
            break;
        } else if (hi_key->dt == dt) {
            result.rotation = hi_key->q;
            break;
        }
    }

    // scale
    result.scale = (sample->scales + (sample->num_scales - 1))->vec;
    for (u32 scale_idx = 0; scale_idx < sample->num_scales; ++scale_idx) 
    {
        dt_v3_Pair *hi_key = sample->scales + scale_idx;
        if (hi_key->dt > dt) {
            dt_v3_Pair *lo_key = (hi_key - 1);
            f32 t = (dt - lo_key->dt) / (hi_key->dt - lo_key->dt);
            result.scale = lerp(lo_key->vec, t, hi_key->vec);
            break;
        } else if (hi_key->dt == dt) {
            result.scale = hi_key->vec;
            break;
        }
    }

    return result;
}

internal void
eval_node(Animation *anim, f32 dt, Node *node)
{
    Node_Hash_Result hash_result = get_sample_index(anim, node->id);
    if (hash_result.found) {
        Sample *sample = (anim->samples + hash_result.idx);
        Xform trs = interpolate_sample(sample, dt);
        node->current_transform = trs_to_transform(trs.translation, trs.rotation, trs.scale);
    }
    else {
        node->current_transform = node->base_transform;
    }
}

internal void
eval(Model* model, Animation* anim, f32 dt, m4x4* out_transforms, b32 do_eval_node)
{
    Eval_Stack stack = {};

    Eval_Stack_Frame *frame = stack.frames;
    for (;;) {
        Node* node = model->nodes + frame->node_id;

        if (frame->next_child_idx == node->child_count) {
            if (stack.top == 0) {
                break;
            } else {
                stack.frames[stack.top--] = {};
            }
        } else {
            if (!frame->global_transform_done) {
                if (do_eval_node) {
                    eval_node(anim, dt, node);
                }
                m4x4 parent_transform = (stack.top != 0) ? stack.frames[stack.top - 1].global_transform : identity();
                m4x4 global_transform = parent_transform * node->current_transform;
                m4x4 final_transform = global_transform * node->offset;

                frame->global_transform = global_transform;
                out_transforms[node->id] = final_transform;

                frame->global_transform_done = true;
            }

            ++stack.top;
            stack.frames[stack.top].node_id = node->child_ids[frame->next_child_idx++];
        }

        frame = stack.frames + stack.top;
    }
}

internal void
interpolate(Model* model, Animation* anim1, f32 dt1, f32 t, Animation* anim2, f32 dt2)
{
    for (s32 id = 0; id < (s32)model->node_count; ++id) {
        Node* node = model->nodes + id;

        Node_Hash_Result res1 = get_sample_index(anim1, id);
        Node_Hash_Result res2 = get_sample_index(anim2, id);

        if (res1.found && res2.found) {
            Sample* sample1 = anim1->samples + res1.idx;
            Sample* sample2 = anim2->samples + res2.idx;

            assert(sample1->id == id && sample1->id == sample2->id);

            Xform trs1 = interpolate_sample(sample1, dt1);
            Xform trs2 = interpolate_sample(sample2, dt2);
            Xform r = interpolate_trs(trs1, t, trs2);
            m4x4 transform = trs_to_transform(r.translation, r.rotation, r.scale);
            node->current_transform = transform;
        } else {
            node->current_transform = node->base_transform;
        }
    }
}
