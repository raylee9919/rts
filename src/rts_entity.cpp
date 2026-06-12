// Copyright Seong Woo Lee. All Rights Reserved.

// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!
// @Todo: CLEANUP STATE MACHINE!!!!!!!!!

//
// Functionalities
//
internal void
entity_attach(Entity* child, Entity* parent, s32 joint_id = -1)
{
    if (!child || !parent) {
        return;
    }

    // @Todo: unsafe af.
    // Update the chain.
    if (child->parent) {
        dll_remove_np(child->parent->first, child->parent->last, child, next_sibling, prev_sibling);
    }
    dll_push_back_np(child->parent->first, child->parent->last, child, next_sibling, prev_sibling);


    child->parent          = parent;
    child->parent_joint_id = joint_id;
}

internal m3x4* get_skinning_matrices(Entity* e)
{
    assert(e);
    return game_state->skinning_matrices + e->index_to_my_skinning_matrices;
}

internal void
entity_clear_path_data(Entity* entity)
{
    entity->waypoint_queue.clear();
    entity->l_points.clear();
    entity->r_points.clear();
    entity->debug_waypoint_queue.clear();
}

internal List <Entity*>
entities_from_min_max_chunk(Arena* arena, u16 min_chunk_x, u16 min_chunk_y, u16 max_chunk_x, u16 max_chunk_y)
{
    List<Entity*> result = {};
    for (u16 chunk_y = min_chunk_y; chunk_y <= max_chunk_y; ++chunk_y) {
        for (u16 chunk_x = min_chunk_x; chunk_x <= max_chunk_x; ++chunk_x) {
            Chunk* chunk = chunk_from_chunk_position(chunk_x, chunk_y);
            for (Entity* entity = chunk->first_entity, *next; entity != nullptr; entity = next) {
                next = entity->next_in_chunk;

                Link <Entity*> *node = (Link <Entity*> *)push_size(arena, sizeof(Link<Entity*>));
                node->data = entity;
                dll_push_back(result.first, result.last, node);
            }
        }
    }
    return result;
}

internal List <Entity*>
entites_from_position_and_radius(v3 position, f32 radius, Arena* arena)
{
    List<Entity*> result = {};
    
    const f32 x = position.x;
    const f32 y = position.z;
    const v2 pos = v2(x, y);

    const f32 min_x = x - radius;
    const f32 max_x = x + radius;
    const f32 min_y = y - radius;
    const f32 max_y = y + radius;

    u16 min_cx, max_cx, min_cy, max_cy;
    chunk_position_from_world_position(min_x, min_y, &min_cx, &min_cy);
    chunk_position_from_world_position(max_x, max_y, &max_cx, &max_cy);

    result = entities_from_min_max_chunk(arena, min_cx, min_cy, max_cx, max_cy);

    return result;
}

internal void
entity_propagate_arrival(Entity* entity)
{
    // Propagate radius
    const f32 r = 1.5f;
    const f32 too_far_threshold = 9.0f;

    const f32 x = entity->position.x;
    const f32 y = entity->position.z;
    const v2 pos = v2(x, y);

    const f32 min_x = x - r;
    const f32 max_x = x + r;
    const f32 min_y = y - r;
    const f32 max_y = y + r;

    u16 min_cx, max_cx, min_cy, max_cy;
    chunk_position_from_world_position(min_x, min_y, &min_cx, &min_cy);
    chunk_position_from_world_position(max_x, max_y, &max_cx, &max_cy);

    for (u16 chunk_y = min_cy; chunk_y <= max_cy; ++chunk_y) {
        for (u16 chunk_x = min_cx; chunk_x <= max_cx; ++chunk_x) {
            Chunk* chunk = chunk_from_chunk_position(chunk_x, chunk_y);
            for (Entity* other = chunk->first_entity, *next; other != nullptr; other = next) {
                next = other->next_in_chunk;

                if (entity->team != other->team) {
                    continue;
                }

                if (entity->id == other->id) {
                    continue;
                }

                if (other->command == ENTITY_CMD_MOVE) {
                    if (other->destination == entity->destination) {
                        f32 dist = distance(other->position, other->destination);
                        if (dist < too_far_threshold) {
                            other->command = ENTITY_CMD_STOP;
                            entity_clear_path_data(entity);

                            // Recursively propagate arrival to nearby entities.
                            entity_propagate_arrival(other);
                        }
                    }
                }
            }
        }
    }
}

internal void
entity_orient_to(Entity* entity, v3 target, f32 dt)
{
    const v3 dir = normalize(target - entity->position);
    const v3 forward = normalize((to_m4x4(entity->orientation) * v4{0,0,1,0}).xyz);
    const f32 c = safe_ratio(dot(forward, dir), length(forward)*length(dir));
    if (c < 1.0f) {
        f32 radian = dt*8.0f;
        if (cross(forward, dir).y < 0.0f) {
            radian = -radian;
        }
        entity->orientation = rotate(entity->orientation, v3{0,1,0}, radian);
    }
}

bool entity_is_dead(Entity* entity) 
{
    if ((entity->command == ENTITY_CMD_DIEING) || (entity->flags & ENTITY_FLAG_DEAD)) {
        return true;
    }
    return false;
}

internal bool
entity_is_targetable(Entity* entity)
{
    if (entity_is_dead(entity)) return false;
    if (!(entity->flags & ENTITY_FLAG_IS_UNIT)) return false;
    return true;
}

internal bool
can_push(Entity* me, Entity* other)
{
    if (other->command == ENTITY_CMD_ATTACK) return false;
    if (me->team != other->team) return false;
    return true;
}

internal void
entity_find_target(Entity* entity, f32 radius, Arena* arena)
{
    auto entities = entites_from_position_and_radius(entity->position, radius, arena);
    f32 min_dist = F32_MAX;

    Entity *attacker = entity_from_id(entity->recent_attacker_id);
    if (attacker && entity_is_targetable(attacker)) {
        min_dist = distance(attacker->position, entity->position);
        entity->command   = ENTITY_CMD_ATTACK;
        entity->target_id = attacker->id;
    }

    for (Link<Entity*> *node = entities.first; node != nullptr; node = node->next) {
        Entity* other = node->data;

        if (other->team == entity->team) {
            continue;
        }

        if (other->id == entity->id) {
            continue;
        }

        if (!entity_is_targetable(other)) {
            continue;
        }

        f32 dist = distance(entity->position, other->position);
        if (dist < radius) {
            if (dist < min_dist) {
                min_dist = dist;
                entity->command   = ENTITY_CMD_ATTACK;
                entity->target_id = other->id;
            }
        }
    }
}

// Modifies 'l_points', 'r_points' and 'waypoint_queue'.
//
internal void
entity_find_path(Entity* entity, v3 destination)
{
    ProfileScope;

    // Alias
    cdt_triangle* triangles = game_state->navmesh.triangles;
    Cdt_Context* ctx = &game_state->navmesh.ctx;
    int num_tri = game_state->navmesh.triangle_count;

    // Find the triangles that contain the source and destination points.
    //
    v2 src = v2(entity->position.z, entity->position.x);
    v2 dst = v2(destination.z, destination.x);

    cdt_triangle src_tri = cdt_get_triangle_containing_point(ctx, src.x, src.y);
    cdt_triangle dst_tri = cdt_get_triangle_containing_point(ctx, dst.x, dst.y);

    int src_idx = -1;
    int dst_idx = -1;

    for (int i = 0; i < num_tri; ++i) {
        if (triangles[i].edges[0] == src_tri.edges[0] ||
            triangles[i].edges[1] == src_tri.edges[0] ||
            triangles[i].edges[2] == src_tri.edges[0]) 
        {
            src_idx = i;
            break;
        }
    }

    for (int i = 0; i < num_tri; ++i) {
        if (triangles[i].edges[0] == dst_tri.edges[0] ||
            triangles[i].edges[1] == dst_tri.edges[0] ||
            triangles[i].edges[2] == dst_tri.edges[0]) 
        {
            dst_idx = i;
            break;
        }
    }

    // A*
    //
    // Uses Euclidean distance to the destination triangle as the heuristic.
    // Uses the sum of distance from each triangle centroid to the shared edge as the edge weight.
    //

    // Preprocess
    f32 unreachable_dist = F32_MAX;
    // @Temporary:
    f32 *f_costs = push_array(game_state->frame_arena, f32, num_tri);
    int *parent = push_array(game_state->frame_arena, int, num_tri);
    for (int i = 0; i < num_tri; ++i) {
        f_costs[i] = unreachable_dist; 
        parent[i] = i;
    }
    f_costs[src_idx] = 0.f;
    parent[src_idx]  = -1;

    Priority_Queue<Pair<f32, int>> open_list = {};
    open_list.push({0.f, src_idx});

    v2 dst_center = v2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333333f, 
                       (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333333f);


    // A* loop: f_cost = g_cost + h_cost(heuristic)
    //
    while (open_list.size > 0) {
        // Pop the shortest in the open list. Implemented with priority queue.
        auto fcost_index = open_list.pop();
        f32 f_cost_cur   = fcost_index.x;
        int idx_cur      = fcost_index.y;

        // Reached the destination triangle.
        if (idx_cur == dst_idx) {
            break; 
        }

        if (f_cost_cur > f_costs[idx_cur]) {
            continue;
        }

        cdt_triangle tri = triangles[idx_cur];
        v2 tri_center = v2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                           (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

        cdt_triangles adj = cdt_get_adjacent_triangles(tri);
        for (int i = 0; i < 3; ++i) {
            cdt_triangle adj_tri = adj.triangles[i];

            cdt_edge* portal_edge = cdt_get_edge(tri.edges[i]);

            // One cannot pass through a solid wall.
            if (cdt_is_constrained(portal_edge)) {
                continue;
            }

            // One cannot pass through a narrow pass.
            v2 p = v2(portal_edge->e[2].org->pos.x, portal_edge->e[2].org->pos.y);
            v2 q = v2(portal_edge->e[0].org->pos.x, portal_edge->e[0].org->pos.y);
            f32 margin = 0.01f;
            if (distance(p,q) < entity->radius*2.f + margin) {
                continue;
            }
            v2 edge_center = (p+q)*0.5f;

            int adj_idx = -1;
            for (int j = 0; j < num_tri; ++j) {
                if (triangles[j].edges[0] == adj_tri.edges[0] ||
                    triangles[j].edges[1] == adj_tri.edges[0] ||
                    triangles[j].edges[2] == adj_tri.edges[0]) 
                {
                    adj_idx = j;
                }
            }
            assert(adj_idx != -1);

            v2 adj_center = v2((adj_tri.x[0] + adj_tri.x[1] + adj_tri.x[2]) * 0.333333f,
                               (adj_tri.y[0] + adj_tri.y[1] + adj_tri.y[2]) * 0.333333f);

            f32 h_cost_cur   = distance(tri_center, dst_center);
            f32 g_cost_cur   = f_cost_cur - h_cost_cur;
            f32 g_cur_to_adj = distance(tri_center, edge_center) + distance(edge_center, adj_center);
            f32 g_cost_adj   = g_cost_cur + g_cur_to_adj;
            f32 h_cost_adj   = distance(adj_center, dst_center);
            f32 f_cost_new   = g_cost_adj + h_cost_adj;

            if (f_costs[adj_idx] > f_cost_new) {
                parent[adj_idx] = idx_cur;
                f_costs[adj_idx]  = f_cost_new;

                Pair<f32, int> new_entry = {f_cost_new, adj_idx};
                open_list.push(new_entry);
            }
        }
    }


    // Gather portal edges' points.
    //
    if (f_costs[dst_idx] != unreachable_dist) {
        entity->l_points.push(dst);
        entity->r_points.push(dst);
        if (src_idx != dst_idx) {
            for (int t = dst_idx; t != src_idx; t = parent[t]) {
                cdt_triangle tri = triangles[t];
                cdt_quad_edge *portal = cdt_get_portal_edge(tri, triangles[parent[t]]);

                v2 r = v2(portal->org->pos.x, portal->org->pos.y);
                v2 l = v2(cdt_sym(portal)->org->pos.x, cdt_sym(portal)->org->pos.y);

                // Deflate the edge widths by the entity's diameter.
                v2 lr = normalize(r-l);
                v2 rl = normalize(l-r);
                l += lr*entity->radius;
                r += rl*entity->radius;

                entity->l_points.push(l);
                entity->r_points.push(r);
            }
            entity->l_points.push(src);
            entity->r_points.push(src);
        }


        // Run the 'Simple Stupid Funnel' and push the resulting waypoints to the queue.
        // https://digestingduck.blogspot.com/2010/03/simple-stupid-funnel-algorithm.html
        //
        // @Todo: Can I do better?
        //

        int portal_count = entity->l_points.num;
        int apex_idx = portal_count - 1;
        int l_idx    = portal_count - 1;
        int r_idx    = portal_count - 1;
        v2 apex  = v2{entity->position.z, entity->position.x};
        v2 l_end = apex;
        v2 r_end = apex;

        entity->waypoint_queue.push(entity->position);
        for (int i = portal_count - 2; i >= 0; --i) {
            v2 l = entity->l_points[i];
            v2 r = entity->r_points[i];

            if (triarea2(l, apex, l_end) <= 0.f) {
                if ((apex.x==l_end.x && apex.y==l_end.y) || (triarea2(l, apex, r_end) > 0.f)) {
                    l_end = l;
                    l_idx = i;
                } else {
                    entity->waypoint_queue.push(v3(r_end.y, 0.f, r_end.x)); // @Hack

                    apex = r_end;
                    apex_idx = r_idx;

                    l_end = apex;
                    r_end = apex;

                    r_idx = apex_idx;
                    l_idx = apex_idx;

                    i = apex_idx;
                    continue;
                }
            }

            if (triarea2(r, apex, r_end) >= 0.f) {
                if ((apex.x==r_end.x && apex.y==r_end.y) || (triarea2(r, apex, l_end) < 0.f)) {
                    r_end = r;
                    r_idx = i;
                } else {
                    entity->waypoint_queue.push(v3(l_end.y, 0.f, l_end.x)); // @Hack

                    apex = l_end;
                    apex_idx = l_idx;

                    r_end = apex;
                    l_end = apex;

                    l_idx = apex_idx;
                    r_idx = apex_idx;

                    i = apex_idx;
                    continue;
                }
            }

        }
        entity->destination = destination;

        entity->waypoint_queue.push(destination);
        entity->debug_waypoint_queue = entity->waypoint_queue;
        entity->waypoint_queue.pop(); // I don't want self position in the queue.
    }
}

// Management
//
internal Entity* 
entity_bucket_from_id(u64 id) 
{
    u64 hashed = XXH3_64bits_withSeed(&id, sizeof(id), 0);
    u64 bucket_index = hashed % game_state->entity_table_size;
    Entity *bucket = game_state->entity_table + bucket_index;
    return bucket;
}

internal Entity* 
entity_from_id(u64 id) 
{
    if (id == 0) {
        return nullptr;
    }

    Entity* result = nullptr;
    Entity* bucket = entity_bucket_from_id(id);

    for (Entity* entity = bucket->first; entity; entity = entity->next_in_table) {
        if (entity->id == id) {
            result = entity;
            break;
        }
    }

    return result;
}

Entity* entity_alloc() 
{
    Entity* entity = game_state->first_free_entity;

    if (entity) {
        sll_pop_front_n(game_state->first_free_entity, game_state->last_free_entity, next_in_table);
        zero_struct(entity);
    } else {
        entity = push_struct(game_state->entity_arena, Entity);
    }

    entity->id = game_state->next_generational_id++;

    Entity* bucket = entity_bucket_from_id(entity->id);

    dll_push_back_np(bucket->first, bucket->last, entity, next_in_table, prev_in_table);
    
    return entity;
}

internal Chunk* 
chunk_from_chunk_position(u16 x, u16 y)
{
    Chunk* result = game_state->chunks + y*game_state->chunk_count_x + x;
    return result;
}

internal void 
chunk_position_from_world_position(f32 world_x, f32 world_y, u16* out_x, u16* out_y)
{
    f32 half_world_x = 0.5f * game_state->map_size.x;
    f32 half_world_y = 0.5f * game_state->map_size.y;

    f32 x = world_x + half_world_x;
    f32 y = world_y + half_world_y;

    u16 chunk_x = (u16)(min(max(x, 0.f), game_state->map_size.x) / game_state->chunk_size.x);
    u16 chunk_y = (u16)(min(max(y, 0.f), game_state->map_size.y) / game_state->chunk_size.y);

    *out_x = chunk_x;
    *out_y = chunk_y;
}

// @Todo: use id.
internal void 
entity_init(Entity* entity, Entity* parent)
{
    if (entity) {
        // Init hierarchy
        //
        if (parent) {
            entity->parent = parent;
        } else {
            entity->parent = game_state->root_entity;
        }
        dll_push_back_np(entity->parent->first, entity->parent->last, entity, next_sibling, prev_sibling);

        // @Todo: Seems alright... but I want the default joint id to be zero.
        //
        entity->parent_joint_id = -1;


        if (entity->flags & ENTITY_FLAG_CHUNK_PARTITIONED) {
            u16 chunk_x, chunk_y;
            chunk_position_from_world_position(entity->position.x, entity->position.z, &chunk_x, &chunk_y);
            Chunk* chunk = chunk_from_chunk_position(chunk_x, chunk_y);

            dll_push_back_np(chunk->first_entity, chunk->last_entity, entity, next_in_chunk, prev_in_chunk);

            entity->chunk_x = chunk_x;
            entity->chunk_y = chunk_y;
        }

        // @Todo: Cursed coordinate..
        //
        if (entity->navmesh_scale > 0.f) {
            const u64 id = entity->id;
            const f32 x = entity->position.z;
            const f32 y = entity->position.x;
            const f32 f = entity->navmesh_scale * 0.5f;
            cdt_insert(&game_state->navmesh.ctx, id, x-f,y+f,x-f,y-f);
            cdt_insert(&game_state->navmesh.ctx, id, x-f,y-f,x+f,y-f);
            cdt_insert(&game_state->navmesh.ctx, id, x+f,y-f,x+f,y+f);
            cdt_insert(&game_state->navmesh.ctx, id, x+f,y+f,x-f,y+f);
        }


        // Update max radius in game.
        game_state->max_radius = max(game_state->max_radius, entity->radius);
    }
}

void entity_release(u64 id) 
{
    Entity* entity = entity_from_id(id);

    if (entity) {

        if (entity->animation_player) {
            release_animation_player(entity->animation_player);
        }

        // Remove from the parent's children list.
        if (entity->parent) {
            dll_remove_np(entity->parent->first, entity->parent->last, entity, next_sibling, prev_sibling);
        }

        // Remove from the table and append to free list.
        Entity* bucket = entity_bucket_from_id(entity->id);
        dll_remove_np(bucket->first, bucket->last, entity, next_in_table, prev_in_table);
        sll_push_front_n(game_state->first_free_entity, game_state->last_free_entity, entity, next_in_table);
    }
}

internal void 
entity_update(Entity* entity, const f32 dt) 
{
    ProfileScopeNC("entity_update", 0x5F96F7);
    
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    switch (entity->type) {
        default: {
            assert(!"INVALID DEFUALT CASE");
        } break;

        case ENTITY_TYPE_ROOT: {
            goto lb_update_children;
        } break;

        case ENTITY_TYPE_CAMERA: {
            if (game_state->controlling_camera_id == entity->id) {
                f32 accel_strength = 50.0f;
                f32 friction       = 7.0f;
                f32 max_speed      = 20.0f;
                m4x4 rotation      = to_m4x4(entity->orientation);
                v3 dir = {};

                if (entity->flags & ENTITY_FLAG_GAME_CAMERA) {

                    dir += os->key_is_down[KEY_UP]    ? v3( 0, 0,-1) : v3{};
                    dir += os->key_is_down[KEY_LEFT]  ? v3(-1, 0, 0) : v3{};
                    dir += os->key_is_down[KEY_DOWN]  ? v3( 0, 0, 1) : v3{};
                    dir += os->key_is_down[KEY_RIGHT] ? v3( 1, 0, 0) : v3{};

                    v2 mouse_pos = os->get_mouse_position(game_state->window_handle);

                    f32 margin = 2.f;
                    if (mouse_pos.x > game_state->window_width - margin)  dir += v3( 1, 0, 0);
                    if (mouse_pos.x < margin)                             dir += v3(-1, 0, 0);
                    if (mouse_pos.y > game_state->window_height - margin) dir += v3( 0, 0, 1);
                    if (mouse_pos.y < margin)                             dir += v3( 0, 0,-1);

                } else if (entity->flags & ENTITY_FLAG_FREE_CAMERA) {

                    dir += os->key_is_down[KEY_W] ? (rotation * V4( 0,  0, -1, 0)).xyz : v3{};
                    dir += os->key_is_down[KEY_A] ? (rotation * V4(-1,  0,  0, 0)).xyz : v3{};
                    dir += os->key_is_down[KEY_S] ? (rotation * V4( 0,  0,  1, 0)).xyz : v3{};
                    dir += os->key_is_down[KEY_D] ? (rotation * V4( 1,  0,  0, 0)).xyz : v3{};
                    dir += os->key_is_down[KEY_Q] ? (rotation * V4( 0, -1,  0, 0)).xyz : v3{};
                    dir += os->key_is_down[KEY_E] ? (rotation * V4( 0,  1,  0, 0)).xyz : v3{};
                   

                    if (os->key_is_down[KEY_SHIFT]) {
                        accel_strength *= 2.f;
                        max_speed      *= 2.f;
                    }

                    // @Hack:
                    local_persist v2 mouse_position_last = {};
                    local_persist b32 dragging = false;

                    for (Os_Event *event = os->event_first, *next; event != NULL; event = next)
                    {
                        next = event->next;

                        if (event->type == OS_EVENT_PRESS && event->key == KEY_MOUSE_LEFT)
                        {
                            os_event_consume(event);
                            mouse_position_last = os->mouse_position_last;
                            dragging = true;
                        }

                        if (event->type == OS_EVENT_MOUSE_MOVE && dragging) {
                            os_event_consume(event);

                            v2 d = 0.5f * dt * (os->mouse_position_last - mouse_position_last);
                            entity->orientation = build_quaternion(v3{0,1,0}, -d.x) * entity->orientation;
                            entity->orientation = build_quaternion((to_m4x4(entity->orientation)*v4{1,0,0,0}).xyz, -d.y) * entity->orientation;
                            mouse_position_last = os->mouse_position_last;
                        }

                        if (event->type == OS_EVENT_RELEASE && event->key == KEY_MOUSE_LEFT) {
                            os_event_consume(event);
                            dragging = false;
                        }
                    }
                } else {
                    assert(!"Invalid camera flag");
                }

                if (sqlen(dir) > 0.0f) {
                    dir = normalize(dir); 
                }

                v3 target_accel = dir * accel_strength;
                entity->velocity += (dt*target_accel);

                if (sqlen(dir) == 0.0f) {
                    entity->velocity -= entity->velocity * friction * dt; 
                }

                f32 speed = length(entity->velocity);
                if (speed > max_speed) {
                    entity->velocity = (entity->velocity / speed) * max_speed; 
                }

                entity->position += (dt*entity->velocity); 
            }

            entity->width = 2.0f;
            f32 h_over_w = (f32)game_state->draw_height / (f32)game_state->draw_width;
            entity->height = entity->width * h_over_w;


            // Build a view matrix from quaternion.
            m4x4 R = to_m4x4(entity->orientation);
            R = transpose(R);
            v3 T = entity->position;
            m4x4 view = R * m4x4_translate(-T.x, -T.y, -T.z);

            f32 f = entity->focal_length;
            f32 N = entity->N;
            f32 F = entity->F;
            f32 a = safe_ratio(2.0f * f, entity->width);
            f32 b = safe_ratio(2.0f * f, entity->height);
            f32 c = (N + F) / (N - F);
            f32 d = (2 * N * F) / (N - F);
            m4x4 proj = {{
                { a,  0,  0,  0},
                { 0,  b,  0,  0},
                { 0,  0,  c,  d},
                { 0,  0, -1,  0}
            }};

            entity->V  = view;
            entity->P  = proj;
            entity->VP = proj * view;

        } break;

        case ENTITY_TYPE_SOLDIER: {

            if ( entity->team == TEAM_PLAYER && !entity_is_dead(entity) && (entity->flags & ENTITY_FLAG_SELECTED) ) {
                for (Os_Event* event = os->event_first, *next; event != nullptr; event = next) {
                    next = event->next;

                    if (event->type == OS_EVENT_PRESS && event->key == KEY_MOUSE_RIGHT) {
                        // @Hack
                        //os_event_consume(event);

                        // Screen space
                        f32 mx = event->position.x;
                        f32 my = event->position.y;

                        Entity* camera = entity_from_id(game_state->controlling_camera_id);

                        v3 dstv3;
                        Ray3 ray = ray_from_screen_position(v2(mx,my), (f32)game_state->window_width, (f32)game_state->window_height, camera->VP);
                        if (ray_plane_intersect(ray, v3{0,1,0}, 0.f, &dstv3)) {
                            // Clear old path data and find new path.
                            entity_clear_path_data(entity);
                            entity_find_path(entity, dstv3);

                            // @Robustness
                            entity->command = ENTITY_CMD_MOVE;
                        }
                    }
                }
            }


            if (entity->hitpoints <= 0.f && entity->command != ENTITY_CMD_DIEING) {
                entity->command = ENTITY_CMD_DIEING;

                if (entity->flags & ENTITY_FLAG_SELECTED) {
                    entity->flags &= (~ENTITY_FLAG_SELECTED);

                    // @Todo: :(
                    auto *gs = game_state;
                    for (Link <u64> *node = gs->selected_entities.first, *next; node; node = next) {
                        next = node->next;

                        if (node->data == entity->id) {
                            dll_remove(gs->selected_entities.first, gs->selected_entities.last, node);
                        }
                    }
                }
            }

            if (entity->command == ENTITY_CMD_STOP) {
                const f32 aggro_radius = 8.f;
                entity_find_target(entity, aggro_radius, scratch.arena);
            } else if (entity->command == ENTITY_CMD_MOVE) {
                const f32 arrival_threshold = 1.0f;
                if (entity->waypoint_queue.empty()) {
                    entity->command = ENTITY_CMD_STOP;
                    entity_propagate_arrival(entity);
                }
            } else if (entity->command == ENTITY_CMD_ATTACK) {
                const f32 attackable_dist = 1.0f;
                const f32 chase_dist      = 16.f;

                Entity* other = entity_from_id(entity->target_id);
                if (other) {
                    if (!entity_is_dead(other)) {
                        // @Robustness
                        f32 dist = distance(entity->position, other->position) - entity->radius - other->radius;
                        if (dist < attackable_dist) {
                            entity_clear_path_data(entity);
                            entity_orient_to(entity, other->position, dt);

                            // Do damage to the target.
                            const f32 damage    = 8.f;   // Damage per hit
                            const f32 damage_t  = entity->damage_t;
                            const f32 period    = entity->attack_max_t;

                            const f32 prev_t = entity->prev_attack_t;
                            const f32 curr_t = prev_t + dt;

                            assert(damage_t < period);

                            // Count crossings
                            const u32 hits =
                                (u32)floor((curr_t - damage_t) / period) -
                                (u32)floor((prev_t - damage_t) / period);

                            if (hits > 0) {
                                const f32 total_damage = (f32)hits * damage;
                                other->hitpoints = max(other->hitpoints - total_damage, 0.f);

                                other->recent_attacker_id = entity->id;
                            }
                        } else {
                            if (entity->find_target_t > entity->find_target_max_t) {
                                entity->find_target_t = 0;
                                const f32 aggro_radius = 8.f;
                                entity_find_target(entity, aggro_radius, scratch.arena);
                            } else if (dist < chase_dist) {
                                entity_clear_path_data(entity);
                                // @Todo: Tick or something. It is ridiculous.
                                entity_find_path(entity, other->position);
                            } else {
                                entity->command = ENTITY_CMD_STOP;
                                entity_clear_path_data(entity);
                            }
                        }
                    } else {
                        entity->target_id = 0;
                        entity->command   = ENTITY_CMD_STOP;
                        entity_clear_path_data(entity);
                    }
                }
            } else if (entity->command == ENTITY_CMD_DIEING) {
                entity_clear_path_data(entity);
                entity->flags &= (~ENTITY_FLAG_COLLIDEABLE);
            }


            entity->prev_attack_t = entity->attack_t;
            if (entity->command == ENTITY_CMD_ATTACK && entity->waypoint_queue.empty()) {
                entity->attack_t = fmod_cycling(entity->attack_t + dt, entity->attack_max_t);
            } else {
                entity->attack_t = 0.f;
            }

            entity->find_target_t = fmod_cycling(entity->find_target_t + dt, entity->find_target_max_t);
            

            // Process waypoint queue.
            if (entity->waypoint_queue.empty()) {
                entity_clear_path_data(entity);
                entity->speed_t = max(entity->speed_t - dt, entity->min_t);
            } else {
                const v3 waypoint = entity->waypoint_queue.front();
                const f32 dist = distance(entity->position, waypoint);
                const f32 arrival_threshold = 1.0f;

                // Check if entity has reached the waypoint.
                if (dist < arrival_threshold) {
                    entity->waypoint_queue.pop();

                    if (entity->waypoint_queue.empty()) {
                        entity_clear_path_data(entity);
                    }
                }

                // Update orientation and speed.
                entity_orient_to(entity, waypoint, dt);
                entity->speed_t = min(entity->speed_t + dt, entity->max_t);
            }


            // Update position. 
            const m4x4 rotation = to_m4x4(entity->orientation);
            const v3 velocity   = (rotation * V4(0, 0, entity->speed, 0)).xyz;
            entity->position    = entity->position + velocity * dt;


            // Update speed.
            {
                const f32 norm_t = map_unorm(entity->speed_t, entity->min_t, entity->max_t);
                entity->speed = hermite(0.f, entity->max_speed, norm_t);
            }



            // Animation
            //
            if (entity->model && entity->skeleton) {
                // @Todo: Sync... Who's responsibility is it??
                Skeleton *sk = entity->skeleton;
                Animation_Player *ap = entity->animation_player;
                if (ap) {
                    if (!entity_is_dead(entity)) {
                        if (entity->attack_t > 0.f) {
                            ap->channels[2].set_animation(entity->attack_animation, true);

                            ap->blend_weights[0] = clamp(ap->blend_weights[0] - dt * 2.f, 0.f, 1.f);
                            ap->blend_weights[1] = clamp(ap->blend_weights[1] - dt * 2.f, 0.f, 1.f);
                            ap->blend_weights[2] = clamp(ap->blend_weights[2] + dt * 2.f, 0.f, 1.f);
                            ap->blend_weights[3] = clamp(ap->blend_weights[3] - dt * 2.f, 0.f, 1.f);
                        } else {
                            ap->channels[0].set_animation(entity->idle_animation, true);
                            ap->channels[1].set_animation(entity->running_animation, true);

                            f32 t = map_unorm(entity->speed, 0.f, entity->max_speed);
                            ap->blend_weights[0] = 1.f - t;
                            ap->blend_weights[1] = t;
                            ap->blend_weights[2] = clamp(ap->blend_weights[2] - dt * 2.f, 0.f, 1.f);
                            ap->blend_weights[3] = clamp(ap->blend_weights[3] - dt * 2.f, 0.f, 1.f);
                        }
                    } else {
                        ap->channels[3].set_animation(entity->die_animation, false);

                        // @Fix: Proper blend...
                        ap->blend_weights[0] = clamp(ap->blend_weights[0] - dt * 2.f, 0.f, 1.f);
                        ap->blend_weights[1] = clamp(ap->blend_weights[1] - dt * 2.f, 0.f, 1.f);
                        ap->blend_weights[2] = clamp(ap->blend_weights[2] - dt * 2.f, 0.f, 1.f);
                        ap->blend_weights[3] = clamp(ap->blend_weights[3] + dt * 2.f, 0.f, 1.f);
                    }
                }
            }
        } break;

        case ENTITY_TYPE_SWORD: {
            // @Todo: Unsafe
            Entity *parent = entity->parent;
            s32 joint_id = entity->parent_joint_id;
            Joint *joint = &parent->skeleton->joints[joint_id];
            m3x4* skinnings = get_skinning_matrices(parent);
            m3x4 joint_3x4 = skinnings[joint_id];
            m4x4 mat;
            {
                mat.rows[0] = joint_3x4.rows[0];
                mat.rows[1] = joint_3x4.rows[1];
                mat.rows[2] = joint_3x4.rows[2];
                mat.rows[3] = v4(0, 0, 0, 1);
            }
            m4x4 local_transform = mat * inverse(joint->inverse_bind_pose);
            m4x4 world_transform = to_m4x4(parent->position, parent->orientation, parent->scaling) * local_transform;
            entity->transform = world_transform * to_m4x4(entity->position, entity->orientation, entity->scaling);
        } break;

        case ENTITY_TYPE_CASTLE: {
        } break;

        case ENTITY_TYPE_KNIGHT: {
        } break;
    }

    if (entity->flags & ENTITY_FLAG_COLLIDEABLE) {
        if (entity->radius > 0.f) {
            // @Temporary
            const f32 margin_radius = game_state->max_radius;
            const f32 r = entity->radius + margin_radius;
            const f32 x = entity->position.x;
            const f32 y = entity->position.z;
            const v2 pos = v2(x, y);

            const f32 min_x = x - r;
            const f32 max_x = x + r;
            const f32 min_y = y - r;
            const f32 max_y = y + r;

            u16 min_cx, max_cx, min_cy, max_cy;
            chunk_position_from_world_position(min_x, min_y, &min_cx, &min_cy);
            chunk_position_from_world_position(max_x, max_y, &max_cx, &max_cy);

            for (u16 chunk_y = min_cy; chunk_y <= max_cy; ++chunk_y) {
                for (u16 chunk_x = min_cx; chunk_x <= max_cx; ++chunk_x) {
                    Chunk* chunk = chunk_from_chunk_position(chunk_x, chunk_y);
                    for (Entity* other = chunk->first_entity, *next; other != nullptr; other = next) {
                        next = other->next_in_chunk;

                        if (entity->id == other->id) {
                            continue;
                        }

                        const f32 other_x      = other->position.x;
                        const f32 other_y      = other->position.z;
                        const v2 other_pos     = v2(other_x, other_y);
                        const f32 epsilon      = 0.001f;

                        if (other->flags & ENTITY_FLAG_COLLIDEABLE) {
                            const f32 other_radius = other->radius;
                            const f32 dist         = distance(pos, other_pos);
                            const f32 radii        = entity->radius + other_radius + epsilon;

                            // colliding
                            if (dist < radii) {
                                const v2 normal = normalize(other_pos - pos); // @Fix: This can give a bogus number!
                                const f32 depth = radii - dist;

                                if (can_push(entity, other)) {
                                    entity->position.x -= normal.x*depth*0.5f;
                                    entity->position.z -= normal.y*depth*0.5f;

                                    other->position.x += normal.x*depth*0.5f;
                                    other->position.z += normal.y*depth*0.5f;
                                } else {
                                    entity->position.x -= normal.x*depth;
                                    entity->position.z -= normal.y*depth;
                                }
                            }
                        } else if (other->navmesh_scale > 0.f) {
                            // nearest position on navmesh's rectangle to unit's circle.
                            // @Fix: WRONG WRONG WRONG
                            f32 half = 0.5f * other->navmesh_scale;
                            f32 dx = clamp(pos.x - other_x, -half, half);
                            f32 dy = clamp(pos.y - other_y, -half, half);

                            if (abs(dx) < half && abs(dy) < half) {
                            } else {
                                // Point on the edge of a rect that's closest to the circle.
                                v2 np = other_pos + v2(dx, dy);
                                v2 dv = np - pos;

                                f32 d = length(dv);
                                f32 radius = entity->radius - 1e-6f;

                                // colliding
                                if (d < radius) {
                                    f32 depth = radius - d;
                                    v2 normal = pos - other_pos;

                                    // resolution
                                    entity->position.x += normal.x*depth;
                                    entity->position.z += normal.y*depth;
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    if (entity->flags & ENTITY_FLAG_CHUNK_PARTITIONED) {
        u16 new_chunk_x, new_chunk_y;
        chunk_position_from_world_position(entity->position.x, entity->position.z, &new_chunk_x, &new_chunk_y);

        if (new_chunk_x != entity->chunk_x || new_chunk_y != entity->chunk_y) {
            Chunk *chunk_old = game_state->chunks + entity->chunk_y*game_state->chunk_count_x + entity->chunk_x;

            for (Entity *it = chunk_old->first_entity, *next; it != NULL; it = next) {
                next = it->next_in_chunk;

                if (it->id == entity->id) {
                    dll_remove_np(chunk_old->first_entity, chunk_old->last_entity, it, next_in_chunk, prev_in_chunk);
                    break;
                }
            }

            Chunk *new_chunk = chunk_from_chunk_position(new_chunk_x, new_chunk_y);
            dll_push_back_np(new_chunk->first_entity, new_chunk->last_entity, entity, next_in_chunk, prev_in_chunk);

            entity->chunk_x = new_chunk_x;
            entity->chunk_y = new_chunk_y;
        }
    }


lb_update_children:
    // DFS
    for (Entity* child = entity->first, *next; child != nullptr; child = next) {
        next = child->next_sibling;
        entity_update(child, dt); 
    }
}

internal void 
entity_draw(Entity* entity, f32 dt, Render_Group* render_group, Render_Commands* commands) 
{
    ProfileScopeNC("entity_draw", 0xFDFBD4);

    switch (entity->type) 
    {
        default: {
            assert(!"Invalid defualt case");
        } break;

        case ENTITY_TYPE_ROOT: {
            goto lb_update_children;
        } break;

        case ENTITY_TYPE_SOLDIER: {
            m4x4 transform = to_m4x4(entity->position, entity->orientation, entity->scaling);
            if (entity->model) {

                for (u32 mesh_idx = 0; mesh_idx < entity->model->num_meshes; ++mesh_idx) {
                    Mesh* mesh = entity->model->meshes + mesh_idx;
                    push_mesh(renderer, mesh, transform, entity->index_to_my_skinning_matrices, entity->skeleton->num_joints, v2(1.f, 1.f));
                }
            }

            commands->debug_transform = transform;
            commands->debug_radius = entity->radius;

            if (!entity->debug_waypoint_queue.empty()) {
                if (commands->draw_navmesh) {
                    // Draw waypoints
                    //
                    for (int i = 0; i < entity->debug_waypoint_queue.count() - 1; ++i) {
                        int idx1 = ((entity->debug_waypoint_queue.front_idx + i) % array_count(entity->debug_waypoint_queue.data));
                        int idx2 = ((entity->debug_waypoint_queue.front_idx + i + 1) % array_count(entity->debug_waypoint_queue.data));
                        v3 p1 = entity->debug_waypoint_queue.data[idx1];
                        v3 p2 = entity->debug_waypoint_queue.data[idx2];

                        f32 alpha = 0.7f;
                        if (entity->team == TEAM_PLAYER) {
                            draw_line(render_group, p1, p2, v4{0.2f,0.2f,1.f,alpha});
                        } else {
                            draw_line(render_group, p1, p2, v4{1.0f,0.2f,0.2f,alpha});
                        }
                    }

                    // Draw portal edges.
                    //
                    //for (int i = 0; i < entity->l_points.count; ++i) {
                    //    v2 l = entity->l_points[i];
                    //    v2 r = entity->r_points[i];
                    //    draw_line(render_group, V3(l.y, 0.2f, l.x), V3(r.y, 0.2f, r.x), v4{1,1,0,1});
                    //}

                }
            }


            Entity* camera = entity_from_id(game_state->controlling_camera_id);
            v3 p = project(entity->position, camera->VP);
            p.x = ( p.x*0.5f + 0.5f)*game_state->window_width;
            p.y = (-p.y*0.5f + 0.5f)*game_state->window_height;

            // Show chunk position of this entity.
            //
            if (!entity_is_dead(entity) && (game_state->display_chunk_position) && (entity->flags & ENTITY_FLAG_CHUNK_PARTITIONED)) {

                auto aabb = fp_draw_string(utf8f(game_state->frame_arena, "[%llu] %.2f", entity->id, entity->attack_t), ui_state->base_family, ui_state->font_size, v2(p.x, p.y), RENDER_STRING_FLAG_NO_DRAW | RENDER_STRING_FLAG_COMPUTE_SIZE).aabb;
                aabb.min -= v2(4.f, 4.f);
                aabb.max += v2(4.f, 4.f);
                v4 c = v4{0.12f, 0.12f, 0.12f, 1.f};
                f32 r = 6.f;
                render_quad_c4r4(aabb.min, aabb.max, c,c,c,c, r,r,r,r);
                fp_draw_string(utf8f(game_state->frame_arena, "[%llu] %.2f", entity->id, entity->attack_t), ui_state->base_family, ui_state->font_size, v2(p.x, p.y), RENDER_STRING_FLAG_COMPUTE_SIZE);
            }


            // Draw hitpoints.
            //
            if ( (entity->flags & ENTITY_FLAG_SELECTED) && (entity->hitpoints > 0.f) ) {
                v2 cen = v2(p.x, p.y);
                v2 dim = v2(18.f, 2.f);
                v2 border = v2(1.f);

                v2 min = cen - dim;
                v2 max = cen + dim;
                render_quad_c(min - border, max + border, v4{0.2f, 0.2f, 0.2f, 0.2f});

                f32 t = map_unorm(entity->hitpoints, 0.f, entity->max_hitpoints);
                v2 max2 = min + v2(2.f * dim.x * t, 2.f * dim.y);
                render_quad_c(min, max2, v4{1.0f, 0.2f, 0.2f, 1.0f});
            }
        } break;

        case ENTITY_TYPE_SWORD: {
            // @Robustness: Most of the entities don't use this 'transform' member.
            m4x4 transform = entity->transform;
            if (entity->model) {
                for (u32 i = 0; i < entity->model->num_meshes; ++i) {
                    Mesh *mesh = &entity->model->meshes[i];
                    push_mesh(renderer, mesh, transform, 0, 0, v2{1,1});
                }
            }
        } break;

        case ENTITY_TYPE_CAMERA: {
        } break;

        case ENTITY_TYPE_CASTLE: {
            m4x4 transform = to_m4x4(entity->position, entity->orientation, entity->scaling);
            if (entity->model) {
                for (u32 mesh_idx = 0; mesh_idx < entity->model->num_meshes; ++mesh_idx) {
                    Mesh *mesh = entity->model->meshes + mesh_idx;
                    push_mesh(renderer, mesh, transform, 0, 0, v2{1,1});
                }
            }
        } break;

        case ENTITY_TYPE_KNIGHT: {
            m4x4 transform = to_m4x4(entity->position, entity->orientation, entity->scaling);
            if (entity->model) {
                for (u32 mesh_idx = 0; mesh_idx < entity->model->num_meshes; ++mesh_idx) {
                    Mesh *mesh = entity->model->meshes + mesh_idx;
                    push_mesh(renderer, mesh, transform, entity->index_to_my_skinning_matrices, entity->skeleton->num_joints, v2{1,1});
                }
            }

#if 0
            if (entity->skeleton) {
                for (u32 i = 0; i < entity->skeleton->num_joints; ++i) {
                    s32 parent = entity->skeleton->joints[i].parent;
                    if (parent >= 0) {
                        m4x4* skinnings = get_skinning_matrices(entity);
                        m4x4 transform1 = transform * skinnings[i] * inverse(entity->skeleton->joints[i].inverse_bind_pose);
                        m4x4 transform2 = transform * skinnings[parent] * inverse(entity->skeleton->joints[parent].inverse_bind_pose);
                        v3 p1 = (transform1 * v4{0,0,0,1}).xyz;
                        v3 p2 = (transform2 * v4{0,0,0,1}).xyz;

                        draw_line(render_group, p1, p2, v4{1.f,0.4f,0.4f,1.f});
                    }
                }
            }
#endif
        } break;
    }


lb_update_children:
    // DFS
    for (Entity* child = entity->first, *next; child != nullptr; child = next) {
        next = child->next_sibling;
        entity_draw(child, dt, render_group, commands);
    }
}

struct Update_Animation_Param 
{
    Animation_Player *ap;
    f32 dt;
};

internal OS_WORK_CALLBACK(update_animation_player_work)
{
    Update_Animation_Param *param_ = (Update_Animation_Param *)param;
    Animation_Player *ap = param_->ap;
    f32 dt = param_->dt;

    ap->accumulate(dt);
    ap->eval();
}
