// Copyright Seong Woo Lee. All Rights Reserved.

global WINDOWPLACEMENT      g_window_placement = {sizeof(g_window_placement)};

Entity* debug_spawn_soldier(f32 x, f32 z, Team team, Game_Assets* assets) 
{
    Entity* soldier            = entity_alloc();
    soldier->type              = ENTITY_TYPE_SOLDIER;
    soldier->flags             = (ENTITY_FLAG_IS_UNIT | ENTITY_FLAG_CHUNK_PARTITIONED |
                                  ENTITY_FLAG_COLLIDEABLE | ENTITY_FLAG_SHOWS_ON_MINIMAP);

    soldier->team              = team;

    soldier->radius            = 0.4f;
    soldier->max_speed         = 3.5f;

    soldier->min_t             = 0.0f;
    soldier->max_t             = 0.5f;


    soldier->max_hitpoints     = 40.f;
    soldier->hitpoints         = soldier->max_hitpoints;

    soldier->find_target_max_t = 0.5f;

    soldier->position          = v3(x, 0.f, z);
    soldier->orientation       = {};
    soldier->scaling           = v3(1.f);

    soldier->model             = assets->skeleton_model;
    soldier->skeleton          = assets->skeleton_skeleton;

    u32 num_joints = soldier->skeleton->num_joints;
    assert(game_state->num_skinning_matrices + num_joints <= game_state->max_skinning_matrices);
    soldier->index_to_my_skinning_matrices = game_state->num_skinning_matrices;
    game_state->num_skinning_matrices += num_joints;

    soldier->idle_animation    = assets->skeleton_idle;
    soldier->running_animation = assets->skeleton_run;
    soldier->die_animation     = assets->skeleton_die;
    soldier->attack_animation  = assets->skeleton_attack;

    // @Todo: sync with anim.
    soldier->attack_max_t      = assets->skeleton_attack->duration;
    soldier->damage_t          = 0.5f;
    assert(soldier->damage_t < soldier->attack_max_t);

    soldier->animation_player = alloc_animation_player();
    soldier->animation_player->init(soldier->skeleton, &game_state->skinning_matrices[soldier->index_to_my_skinning_matrices]);

    Entity* parent = nullptr;
    entity_init(soldier, parent);

    return soldier;
}

Entity* debug_spawn_knight(f32 x, f32 z, Team team, Game_Assets* assets) 
{
    Entity* e = entity_alloc();
    e->type   = ENTITY_TYPE_SOLDIER;
    e->flags  = (ENTITY_FLAG_IS_UNIT | ENTITY_FLAG_CHUNK_PARTITIONED |
                 ENTITY_FLAG_COLLIDEABLE | ENTITY_FLAG_SHOWS_ON_MINIMAP);

    e->team = team;

    e->radius    = 0.4f;
    e->max_speed = 3.5f;

    e->min_t = 0.0f;
    e->max_t = 0.5f;

    e->max_hitpoints = 40.f;
    e->hitpoints = e->max_hitpoints;

    e->find_target_max_t = 0.5f;

    e->position    = v3(x, 0.f, z);
    e->orientation = {};
    e->scaling     = v3(1.f);

    e->model    = assets->knight_model;
    e->skeleton = assets->knight_skeleton;

    u32 num_joints = e->skeleton->num_joints;
    assert(game_state->num_skinning_matrices + num_joints <= game_state->max_skinning_matrices);
    e->index_to_my_skinning_matrices = game_state->num_skinning_matrices;
    game_state->num_skinning_matrices += num_joints;

    e->idle_animation    = assets->knight_idle;
    e->running_animation = assets->knight_run;
    e->die_animation     = assets->knight_die;
    e->attack_animation  = assets->knight_attack;

    // @Todo: sync with anim.
    e->attack_max_t      = e->attack_animation->duration;
    e->damage_t          = 0.5f;
    assert(e->damage_t < e->attack_max_t);

    e->animation_player = alloc_animation_player();
    e->animation_player->init(e->skeleton, &game_state->skinning_matrices[e->index_to_my_skinning_matrices]);

    // @Hack:

    Entity* parent = nullptr;
    entity_init(e, parent);

    return e;
}

Entity* debug_attach_sword(Entity* entity, Game_Assets* assets) 
{
    // Create a sword and attach it to soldier.
    //
    Entity* sword = entity_alloc();
    sword->type              = ENTITY_TYPE_SWORD;
    sword->position          = v3(0.0f, 0.0f, -50.0f);
    sword->orientation       = normalize(Quaternion(0.44f, 0.51f, -0.57f, 0.46f));

    // @Todo: Since the current socket's scale is also transformed along the skeleton hierarhcy, the scaling is amplified, hard-coded here.
    sword->scaling           = v3(70.f);
    sword->local_position    = v3(0.f, 0.f, 0.f);
    sword->local_orientation = euler_to_quaternion(radian_from_degree(180.f), 0.f, 0.f);
    sword->model             = game_state->assets->sword_model;
    entity_init(sword, entity);

    // @Temporary
    const s32 joint_id = 47;
    entity_attach(sword, entity, joint_id);

    return sword;
}

Entity* debug_spawn_castle(f32 x, f32 z, Team team, Game_Assets* assets) 
{
    Entity* castle = entity_alloc();
    castle->type   = ENTITY_TYPE_CASTLE;
    castle->flags  = ENTITY_FLAG_CHUNK_PARTITIONED | ENTITY_FLAG_SHOWS_ON_MINIMAP;

    castle->position    = v3(x, 0.f, z);
    castle->orientation = Quaternion(1,0,0,0);
    castle->scaling     = v3(1.f);
    castle->model       = assets->castle_model;

    castle->navmesh_scale = 6.f;

    entity_init(castle, nullptr);

    return castle;
}

Mesh *mesh_from_name(Model *model, Utf8 name)
{
    for (u32 i = 0; i < model->num_meshes; ++i) {
        Mesh *mesh = &model->meshes[i];
        Utf8 n = mesh->name;
        if (n == name) {
            return mesh;
        }
    }
    return NULL;
}

struct Token {
    u8 scratch_buffer[64];
    u8 length;
};

struct Material_Parser {
    u8* contents;
    u64 size;
    u64 cursor;

    u8 peek() {
        return contents[cursor];
    }

    u8 eat() {
        return contents[cursor++];
    }

    bool is_whitespace(u8 c) {
        return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    }

    void eat_whitespace() {
        if (cursor < size) {
            u8 c = peek();

            while (is_whitespace(c)) {
                eat();
                c = peek();
            }
        }
    }

    Token parse_identifier() {
        if (cursor < size) {
            eat_whitespace();
            Token token = {};
            while (!is_whitespace(peek())) {
                token.scratch_buffer[token.length++] = eat();
            }
            return token;
        } else {
            return {};
        }
    }

    Token parse_field_identifier() {
        if (cursor < size) {
            eat_whitespace();
            assert(eat() == ';');
            Token identifier = parse_identifier();
            return identifier;
        } else {
            return {};
        }
    }
};

Material load_material(Asset::System* asset_system, Utf8 asset_dir, Utf8 path) {
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Material material = {};

    Utf8 entire_file = read_entire_file(scratch.arena, path);

    Material_Parser p;
    {
        p.contents = entire_file.str;
        p.size     = entire_file.len;
        p.cursor   = 0;
    }

    while (p.cursor < p.size) {
        p.eat_whitespace();

        // albedo? normal? ..
        Token key = p.parse_field_identifier();
        char* str = (char*)key.scratch_buffer;
        u64 len = key.length;

        if (p.cursor < p.size) {

            Token value = {};
            bool valid = false;
            PBR_Texture_Type slot = PBR_ALBEDO;

            if (string_equal("albedo", str, len)) {
                value =  p.parse_identifier();
                valid = true;
                slot = PBR_ALBEDO;
            } else if (string_equal("normal", str, len)) {
                value =  p.parse_identifier();
                valid = true;
                slot = PBR_NORMAL;
            } else if (string_equal("roughness", str, len)) {
                value =  p.parse_identifier();
                valid = true;
                slot = PBR_ROUGHNESS;
            } else if (string_equal("metallic", str, len)) {
                value =  p.parse_identifier();
                valid = true;
                slot = PBR_METALLIC;
            } else if (string_equal("emission", str, len)) {
                value =  p.parse_identifier();
                valid = true;
                slot = PBR_EMISSION;
            } else {
                assert(!"Unknown field.");
            }

            if (valid) {
                Utf8 texture_path = utf8f(scratch.arena, "%S/textures/%s.texture", asset_dir, value);
                load_texture(asset_system, &material.textures[slot], texture_path);
            }
        }
    }

    return material;
}
