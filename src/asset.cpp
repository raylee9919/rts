// Copyright Seong Woo Lee. All Rights Reserved.



#define READ(to, type)\
    memcpy(&to, at, sizeof(type)); \
    at += sizeof(to);
#define READ_COUNT(to, type, count) \
    to = push_array(arena, type, count); \
    memcpy(to, at, sizeof(type)*count); \
    at += (sizeof(type)*count);

struct Asset_Loader {
    u8 *cursor;
    u8 *end;

    void eat_whitespace() {
        while (cursor < end) {
            u8 c = *cursor;
            if (!is_whitespace(c)) break;
            cursor++;
        }
    }

    u8 peek() {
        assert( cursor && cursor < end );
        return *cursor;
    }

    u8 eat() {
        assert( cursor && cursor < end );
        u8 c = *cursor++;
        return c;
    }

    u32 parse_u32() {
        assert( cursor && cursor < end );
        eat_whitespace();

        u32 result = 0;

        while (cursor < end) {
            u8 c = peek();
            if (is_digit(c)) {
                u32 num = atoi(c);
                result *= 10;
                result += num;
                cursor++;
            } else {
                break;
            }
        }

        return result;
    }

    s32 parse_s32() {
        assert( cursor && cursor < end );
        eat_whitespace();

        bool sign = false;
        u8 sign_char = peek();
        if (sign_char == '+') {
            eat();
        } else if (sign_char == '-') {
            sign = true;
            eat();
        }

        u32 integer = parse_u32();
        assert( integer <= 0x0fffffff );
        s32 result = (u32)integer;

        if (sign) {
            result = -result;
        }

        return result;
    }

    f32 parse_f32() {
        assert( cursor && cursor < end );
        eat_whitespace();

        bool sign = false;
        u8 sign_char = peek();
        if (sign_char == '+') {
            eat();
        } else if (sign_char == '-') {
            sign = true;
            eat();
        }

        s32 integer = 0;
        while (cursor < end) {
            u8 c = peek();
            if (is_digit(c)) {
                s32 num = c - '0';
                integer *= 10;
                integer += num;
                cursor++;
            } else {
                break;
            }
        }

        f32 fraction = 0.f;
        f32 weight = 0.1f;
        if (peek() == '.') {
            cursor++;

            while (cursor < end) {
                char c = peek();
                if (is_digit(c)) {
                    f32 num = (f32)(c - '0');
                    fraction += (num*weight);
                    weight *= 0.1f;
                    cursor++;
                } else {
                    break;
                }
            }
        }

        f32 result = (f32)integer + fraction;
        if (sign) {
            result = -result;
        }
        return result;
    }

    v2 parse_v2() {
        v2 result;
        result.x = parse_f32();
        result.y = parse_f32();
        return result;
    }

    v3 parse_v3() {
        v3 result;
        result.x = parse_f32();
        result.y = parse_f32();
        result.z = parse_f32();
        return result;
    }

    v4 parse_v4() {
        v4 result;
        result.r = parse_f32();
        result.g = parse_f32();
        result.b = parse_f32();
        result.a = parse_f32();
        return result;
    }

    m4x4 parse_m4x4() {
        m4x4 result;
        result.rows[0] = parse_v4();
        result.rows[1] = parse_v4();
        result.rows[2] = parse_v4();
        result.rows[3] = parse_v4();
        return result;
    }

    Quaternion parse_quaternion() {
        Quaternion result;
        result.w = parse_f32();
        result.x = parse_f32();
        result.y = parse_f32();
        result.z = parse_f32();
        return result;
    }

    Asset_Name parse_name(u8 length) {
        Asset_Name result = {};
        assert(length + 1 <= array_count(result.text));

        assert( cursor && cursor < end );
        eat_whitespace();

        result.length = length;
        memcpy(result.text, cursor, length);
        result.text[length] = 0;

        cursor += length;

        return result;
    }
};

internal void load_model(Arena *arena, Model *model_out, Utf8 file_path, v3 scale)
{
    assert(model_out);

    f32 counter = os->perf_counter();
    defer(printf("Took %.6f seconds.\n", (f32)(os->perf_counter() - counter) * os->perf_counter_freq_inv));

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);
    Asset_Loader l = {};
    l.cursor = entire_file.str;
    l.end = entire_file.str + entire_file.len;

    printf("\nLoading model '%s'\n", file_path.str);

    u32 num_meshes = l.parse_u32();
    model_out->num_meshes = num_meshes;
    model_out->meshes = push_array(arena, Mesh, num_meshes);

    printf("Number of meshes: '%u'\n", num_meshes);

    u32 num_total_vertices = 0;

    for (u32 mi = 0; mi < num_meshes; ++mi) {
        Mesh *mesh = &model_out->meshes[mi];

        u8 str_len = (u8)l.parse_u32();
        mesh->name = l.parse_name(str_len);


        // Parse vertices.
        //
        u32 num_vertices = l.parse_u32();
        mesh->num_vertices = num_vertices;
        mesh->vertices = push_array(arena, Vertex, num_vertices);

        num_total_vertices += num_vertices;

        printf("Mesh #%u '%s' has %u vertices.\n", mi, mesh->name.text, num_vertices);

        for (u32 vi = 0; vi < num_vertices; ++vi) {
            Vertex *vert = &mesh->vertices[vi];
            vert->position = hadamard(l.parse_v3(), scale);
            vert->normal   = l.parse_v3();
            vert->uv       = l.parse_v2();
            vert->color    = l.parse_v4();
            vert->tangent  = l.parse_v3();

            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) vert->node_ids[i] = l.parse_s32();
            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) vert->node_weights[i] = l.parse_f32();
        }

        // Parse indices.
        //
        u32 num_indices = l.parse_u32();
        mesh->num_indices = num_indices;
        mesh->indices = push_array(arena, u32, num_indices);

        for (u32 ii = 0; ii < num_indices; ++ii) {
            mesh->indices[ii] = l.parse_u32();
        }
    }

    printf("Total number of vertices: %u\n", num_total_vertices);

    l.eat_whitespace();
    assert( l.cursor == l.end );
}

internal void load_skeleton(Arena *arena, Skeleton *skel_out, Utf8 file_path)
{
    assert(skel_out);

    f32 counter = os->perf_counter();
    defer(printf("Took %.6f seconds.\n", (f32)(os->perf_counter() - counter) * os->perf_counter_freq_inv));

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);
    Asset_Loader l = {};
    l.cursor = entire_file.str;
    l.end = entire_file.str + entire_file.len;

    printf("\nLoading skeleton '%s'\n", file_path.str);

    u32 num_joints = l.parse_u32();
    skel_out->num_joints = num_joints;
    skel_out->joints = push_array(arena, Joint, num_joints);

    for (u32 ji = 0; ji < num_joints; ++ji) {
        Joint *joint = &skel_out->joints[ji];

        u8 name_len = (u8)l.parse_u32();
        joint->name = l.parse_name(name_len);

        s32 parent = l.parse_s32();
        joint->parent = parent;
        joint->local_transform = l.parse_m4x4();
        joint->inverse_bind_pose = l.parse_m4x4();
    }

    l.eat_whitespace();
    assert( l.cursor == l.end );
}

internal u64 hash_joint_id(s32 id)
{
    u64 result = XXH3_64bits_withSeed(&id, sizeof(id), 0);
    return result;
}

internal void load_animation(Arena *arena, Animation *anim_out, Utf8 file_path)
{
    assert(anim_out);

    f32 counter = os->perf_counter();
    defer(printf("Took %.6f seconds.\n", (f32)(os->perf_counter() - counter) * os->perf_counter_freq_inv));

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);
    Asset_Loader l = {};
    l.cursor = entire_file.str;
    l.end = entire_file.str + entire_file.len;

    printf("\nLoading animation '%s'\n", file_path.str);

    u8 name_len = (u8)l.parse_u32();
    anim_out->name = l.parse_name(name_len);

    f32 duration      = l.parse_f32();
    u32 num_keyframes = l.parse_u32();
    u32 num_joints    = l.parse_u32();

    assert( duration != 0.f ); // It'll cause divide by zero.
    anim_out->duration = duration;
    anim_out->num_keyframes = num_keyframes;
    anim_out->num_joints = num_joints;
    anim_out->joints = push_array(arena, Animation_Joint, num_joints);

    for (u32 ji = 0; ji < anim_out->num_joints; ++ji) {
        Animation_Joint *joint = &anim_out->joints[ji];
        joint->id = l.parse_s32();
        joint->keyframes = push_array(arena, Xform, anim_out->num_keyframes);
        for (u32 ki = 0; ki < anim_out->num_keyframes; ++ki) {
            Xform *keyframe = &joint->keyframes[ki];
            keyframe->translation = l.parse_v3();
            keyframe->rotation    = l.parse_quaternion();
            keyframe->scale       = l.parse_v3();
        }
    }

    
    u32 table_size = 256;
    anim_out->table_size = table_size;
    anim_out->joint_table = push_array(arena, Animation_Joint_Entry, table_size);

    for (u32 ji = 0; ji < anim_out->num_joints; ++ji) {
        Animation_Joint *joint = &anim_out->joints[ji];
        s32 id = joint->id;
        u64 slot = hash_joint_id(id) % table_size;

        Animation_Joint_Entry *new_node = push_struct(arena, Animation_Joint_Entry);
        new_node->joint = joint;

        Animation_Joint_Entry *entry = anim_out->joint_table + slot;
        sll_push_back(entry->first, entry->last, new_node);
    }


    l.eat_whitespace();
    assert( l.cursor == l.end );
}

internal void asset_load_image(Bitmap *bitmap, Utf8 file_path, Arena *arena)
{
    assert(bitmap);

    f32 counter = os->perf_counter();
    defer(printf("Took %.6f seconds.\n", (f32)(os->perf_counter() - counter) * os->perf_counter_freq_inv));

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 entire_file = read_entire_file(scratch.arena, file_path);
    u8 *at  = entire_file.str;
    u8 *end = at + entire_file.len;

    printf("\nLoading image '%s'\n", file_path.str);

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

internal void asset_load_image_general_format(Bitmap *bitmap, Utf8 file_path, Arena *arena)
{
    assert(bitmap);

    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    f32 counter = os->perf_counter();
    defer(printf("Took %.6f seconds.\n", (f32)(os->perf_counter() - counter) * os->perf_counter_freq_inv));

    Utf8 contents = read_entire_file(scratch.arena, file_path);

    printf("\nLoading image '%s'\n", file_path.str);

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
