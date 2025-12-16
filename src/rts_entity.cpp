/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal Entity *entity_bucket_from_id(u64 id) {
    u64 hashed = XXH3_64bits_withSeed(&id, sizeof(id), 0);
    u64 bucket_index = hashed % game_state->entity_table_size;
    Entity *bucket = game_state->entity_table + bucket_index;
    return bucket;
}

internal Entity *entity_from_id(u64 id) {
    Entity *result = 0;
    Entity *bucket = entity_bucket_from_id(id);

    for (Entity *entity = bucket->first; entity; entity = entity->next) {
        if (entity->id == id) {
            result = entity;
            break;
        }
    }

    return result;
}

internal Entity *entity_alloc(void) {
    Entity *entity = game_state->first_free_entity;

    if (entity) {
        zero_struct(entity);
        sll_pop_front(game_state->first_free_entity, game_state->last_free_entity);
    } else {
        entity = push_struct(game_state->entity_arena, Entity);
    }

    entity->id = game_state->next_generational_id++;

    Entity *bucket = entity_bucket_from_id(entity->id);

    dll_push_back(bucket->first, bucket->last, entity);
    
    return entity;
}

internal void entity_release(u64 id) {
    Entity *entity = entity_from_id(id);

    if (entity) {
        Entity *bucket = entity_bucket_from_id(entity->id);
        dll_remove(bucket->first, bucket->last, entity);
        sll_push_front(game_state->first_free_entity, game_state->last_free_entity, entity);
    }
}

internal void entity_update(Entity *entity, f32 dt) {
    u32 id = entity->id;

    switch (entity->type) {
        default: {
            assert("INVALID DEFUALT CASE");
        }break;

        case ENTITY_TYPE_CAMERA: {
            if (game_state->controlling_camera_id == id) {
                f32 accel_strength = 50.0f;
                f32 friction       = 7.0f;
                f32 max_speed      = 20.0f;
                m4x4 rotation      = quaternion_to_m4x4(entity->orientation);
                v3 desired_dir     = {};

                desired_dir += os->key_is_down[OS_KEY_W] ? (rotation * V4( 0,  0, -1, 0)).xyz : v3{};
                desired_dir += os->key_is_down[OS_KEY_A] ? (rotation * V4(-1,  0,  0, 0)).xyz : v3{};
                desired_dir += os->key_is_down[OS_KEY_S] ? (rotation * V4( 0,  0,  1, 0)).xyz : v3{};
                desired_dir += os->key_is_down[OS_KEY_D] ? (rotation * V4( 1,  0,  0, 0)).xyz : v3{};
                desired_dir += os->key_is_down[OS_KEY_Q] ? (rotation * V4( 0, -1,  0, 0)).xyz : v3{};
                desired_dir += os->key_is_down[OS_KEY_E] ? (rotation * V4( 0,  1,  0, 0)).xyz : v3{};

                if (os->key_is_down[OS_KEY_SHIFT]) {
                    accel_strength *= 2.f;
                    max_speed      *= 2.f;
                }

                if (sqlen(desired_dir) > 0.0f) {
                    desired_dir = normalize(desired_dir); 
                }

                v3 target_accel = desired_dir * accel_strength;
                entity->velocity += (dt*target_accel);

                if (sqlen(desired_dir) == 0.0f) {
                    entity->velocity -= entity->velocity * friction * dt; 
                }

                f32 speed = length(entity->velocity);
                if (speed > max_speed) {
                    entity->velocity = (entity->velocity / speed) * max_speed; 
                }

                entity->position += (dt*entity->velocity); 

                // HACK:
                local_persist v2 mouse_position_last = {};
                local_persist b32 dragging = false;

                for (Os_Event *event = os->event_first, *next; event != NULL; event = next)
                {
                    next = event->next;

                    if (event->type == OS_EVENT_PRESS && event->key == OS_KEY_MOUSE_LEFT)
                    {
                        os_event_consume(event);
                        mouse_position_last = os->mouse_position_last;
                        dragging = true;
                    }

                    if (event->type == OS_EVENT_MOUSE_MOVE && dragging)
                    {
                        os_event_consume(event);

                        v2 d = 0.5f * dt * (os->mouse_position_last - mouse_position_last);
                        entity->orientation = build_quaternion(v3{0,1,0}, -d.x) * entity->orientation;
                        entity->orientation = build_quaternion((quaternion_to_m4x4(entity->orientation)*v4{1,0,0,0}).xyz, -d.y) * entity->orientation;
                        mouse_position_last = os->mouse_position_last;
                    }

                    if (event->type == OS_EVENT_RELEASE && event->key == OS_KEY_MOUSE_LEFT)
                    {
                        os_event_consume(event);
                        dragging = false;
                    }
                }
            }

            entity->width = 2.0f;
            f32 h_over_w = (f32)game_state->draw_height / (f32)game_state->draw_width;
            entity->height = entity->width * h_over_w;

            m4x4 V = view_transform(entity->position, entity->orientation);
            f32 f = entity->focal_length;
            f32 N = entity->N;
            f32 F = entity->F;
            f32 a = safe_ratio(2.0f * f, entity->width);
            f32 b = safe_ratio(2.0f * f, entity->height);
            f32 c = (N + F) / (N - F);
            f32 d = (2 * N * F) / (N - F);
            m4x4 P = {{
                { a,  0,  0,  0},
                { 0,  b,  0,  0},
                { 0,  0,  c,  d},
                { 0,  0, -1,  0}
            }};

            entity->V = V;
            entity->P = P;
            entity->VP = P*V;

        }break;

        case ENTITY_TYPE_SOLDIER: {
            for (Os_Event *event = os->event_first, *next; event != NULL; event = next) {
                next = event->next;

                if (event->type == OS_EVENT_PRESS && event->key == OS_KEY_MOUSE_RIGHT) {
                    os_event_consume(event);

                    entity->command = ENTITY_CMD_MOVE;

                    // Client space
                    f32 mx = event->position.x;
                    f32 my = event->position.y;

                    // @Todo: Graphics API-independent
                    // To NDC
                    f32 x = 2.f*( mx / (f32)game_state->window_width ) - 1.f;
                    f32 y = 2.f*(-my / (f32)game_state->window_height) + 1.f;

                    Entity *camera = entity_from_id(game_state->controlling_camera_id);
                    m4x4 inv_view_proj = inverse(camera->VP);

                    // @Todo: Graphics API-independent
                    v4 near_clip = v4{x, y, -1.f, 1.f};
                    v4 far_clip  = v4{x, y,  1.f, 1.f};

                    v4 near_p = inv_view_proj*near_clip;
                    v4 far_p  = inv_view_proj*far_clip;

                    near_p.xyz = near_p.xyz / near_p.w;
                    far_p.xyz  = far_p.xyz  / far_p.w;

                    // Define a ray.
                    v3 o = near_p.xyz;
                    v3 v = normalize(far_p.xyz - near_p.xyz);

                    // Define a plane.
                    v3 n  = v3{0,1,0};
                    f32 d = 0.f;

                    // Ray-Plane intersection.
                    f32 t = 0.f;
                    f32 denom = dot(v, n);

                    // If ray is not parallel to the plane, we can update the waypoint queue.
                    if (absolute(denom) > 0.0001f) {
                        t = -(dot(o, n) + d) / denom;
                        v3 dstv3 = o + t*v;

                        // Clear old path data.
                        //
                        entity->waypoint_queue.clear();
                        entity->l_points.clear();
                        entity->r_points.clear();
                        entity->debug_waypoint_queue.clear();

                        // Alias
                        cdt_triangle *triangles = game_state->navmesh.triangles;
                        Cdt_Context *ctx = &game_state->navmesh.ctx;
                        int num_tri = game_state->navmesh.triangle_count;

                        // Find the triangles that contain the source and destination points.
                        //
                        v2 src = v2{entity->position.z, entity->position.x};
                        v2 dst = v2{dstv3.z, dstv3.x};

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
                        parent[src_idx] = -1;

                        Priority_Queue<Pair<f32, int>> open_list = {};
                        open_list.push({0.f, src_idx});

                        v2 dst_center = V2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f, 
                                           (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f);


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
                            v2 tri_center = V2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                               (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                            cdt_triangles adj = cdt_get_adjacent_triangles(tri);
                            for (int i = 0; i < 3; ++i) {
                                cdt_triangle adj_tri = adj.triangles[i];

                                cdt_edge *portal_edge = cdt_get_edge(tri.edges[i]);

                                // One cannot pass through a solid wall.
                                if (cdt_is_constrained(portal_edge)) {
                                    continue;
                                }

                                // One cannot pass through a narrow pass.
                                v2 p = V2(portal_edge->e[2].org->pos.x, portal_edge->e[2].org->pos.y);
                                v2 q = V2(portal_edge->e[0].org->pos.x, portal_edge->e[0].org->pos.y);
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

                                v2 adj_center = V2((adj_tri.x[0] + adj_tri.x[1] + adj_tri.x[2]) * 0.333333f,
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

                                    v2 r = V2(portal->org->pos.x, portal->org->pos.y);
                                    v2 l = V2(cdt_sym(portal)->org->pos.x, cdt_sym(portal)->org->pos.y);

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

                            int portal_count = entity->l_points.count;
                            int apex_idx = portal_count - 1;
                            int l_idx    = portal_count - 1;
                            int r_idx    = portal_count - 1;
                            v2 apex  = {entity->position.z, entity->position.x};
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
                                        entity->waypoint_queue.push(V3(r_end.y, 0.f, r_end.x)); // @Hack

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
                                        entity->waypoint_queue.push(V3(l_end.y, 0.f, l_end.x)); // @Hack

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
                            entity->waypoint_queue.push(dstv3);
                        }

                        entity->debug_waypoint_queue = entity->waypoint_queue;
                    }
                }
            }

            // Move
            //
            if (entity->command == ENTITY_CMD_MOVE) 
            {
                if (!entity->waypoint_queue.empty()) 
                {
                    v3 waypoint = entity->waypoint_queue.front();
                    f32 dist = distance(entity->position, waypoint);
                    if (waypoint == entity->destination) 
                    {
                        // @Fix: twitching character when he starts near the destination point.
                        f32 stop_radius = 1.0f;
                        if (dist > stop_radius) {
                            v3 dir = normalize(waypoint - entity->position);
                            v3 forward = normalize((quaternion_to_m4x4(entity->orientation) * v4{0,0,1,0}).xyz);
                            f32 c = safe_ratio(dot(forward, dir), length(forward)*length(dir));
                            if (c < 1.0f) {
                                f32 radian = dt*10.0f;
                                if (cross(forward, dir).y < 0.0f) {
                                    radian = -radian;
                                }
                                entity->orientation = rotate(entity->orientation, v3{0,1,0}, radian);
                            }

                            entity->transition_t += 2.0f*dt;
                            entity->transition_t = clamp01(entity->transition_t);
                        } else {
                            f32 t = map01(dist, 0.0f, stop_radius);
                            entity->transition_t = lerp(0.0f, t, 0.7f);
                            entity->waypoint_queue.pop();
                        }
                    } else {
                        f32 waypoint_reached_radius = 0.20f;
                        if (dist > waypoint_reached_radius) {
                            v3 dir = normalize(waypoint - entity->position);
                            v3 forward = normalize((quaternion_to_m4x4(entity->orientation) * v4{0,0,1,0}).xyz);
                            f32 c = safe_ratio(dot(forward, dir), length(forward)*length(dir));
                            if (c < 1.0f) {
                                f32 radian = dt*10.0f;
                                if (cross(forward, dir).y < 0.0f) {
                                    radian = -radian;
                                }
                                entity->orientation = rotate(entity->orientation, v3{0,1,0}, radian);
                            }

                            entity->transition_t += 2.0f*dt;
                            entity->transition_t = clamp01(entity->transition_t);
                        } else {
                            entity->waypoint_queue.pop();
                        }
                    }
                } else {
                    entity->command = ENTITY_CMD_IDLE;
                }
            }


            // Animation, Position
            //
            if (! (entity->flags & ENTITY_FLAG_DEAD)) 
            {
                f32 norm = smoothstep(entity->transition_t, 0, 1);
                m4x4 rotation = quaternion_to_m4x4(entity->orientation);
                f32 T = lerp(0.0f, entity->transition_t, 1.0f);
                entity->position = entity->position + dt*entity->velocity;
                entity->velocity = (rotation * V4(0, 0, norm*entity->speed, 0)).xyz;

                entity->transition_t -= dt;
                entity->transition_t = clamp01(entity->transition_t);

                if (entity->model) 
                {
                    f32 v = length(entity->velocity);
                    f32 lo = 0.0001f;
                    f32 hi = 0.7f;
                    Animation_Channel *channel = &entity->animation_channels[0];

                    if (v <= lo) 
                    {
                        Animation *new_anim = entity->idle_animation;
                        if (channel->animation != new_anim) 
                        {
                            channel->animation = new_anim;
                            channel->dt = 0.0f;
                        }
                        eval(entity->model, channel->animation, channel->dt, entity->animation_transform, true);
                        anim_accumulate(channel, dt);
                    } 
                    else if (v > hi) 
                    {
                        Animation *new_anim = entity->running_animation;
                        if (channel->animation != new_anim) 
                        {
                            channel->animation = new_anim;
                            channel->dt = 0.0f;
                        }
                        eval(entity->model, channel->animation, channel->dt, entity->animation_transform, true);
                        anim_accumulate(channel, dt);
                    } 
                    else 
                    {
                        f32 t = map01(v, lo, hi);
                        if (channel->animation == entity->idle_animation) 
                        {
                            interpolate(entity->model, channel->animation, channel->dt, t, entity->running_animation, 0.0f);
                        } 
                        else 
                        {
                            interpolate(entity->model, entity->idle_animation, 0.0f, t, channel->animation, channel->dt);
                        }
                        eval(entity->model, 0, 0, entity->animation_transform, false);
                    }
                }
            } else if (entity->command = ENTITY_CMD_DIEING) {
                Animation_Channel *channel = &entity->animation_channels[0];

                f32 lo = 0.0f;
                f32 hi = 0.1f;
                f32 t = map(entity->transition_t, lo, hi);

                if (t < 1.0f) {
                    interpolate(entity->model, channel->animation, channel->dt, t, entity->die_animation, 0.0f);
                    eval(entity->model, 0, 0, entity->animation_transform, false);
                } else {
                    eval(entity->model, entity->die_animation, entity->transition_t - hi, entity->animation_transform, true);
                    if (entity->transition_t >= entity->die_animation->duration) {
                        entity->flags |= ENTITY_FLAG_DEAD;
                    }
                }
                entity->transition_t += dt;
            } else if (entity->flags & ENTITY_FLAG_DEAD) {
                eval(entity->model, entity->die_animation, entity->die_animation->duration, entity->animation_transform, true);
            } else {
                INVALID_CODE_PATH;
            }
        }break;
    }

    if (entity->flags & ENTITY_FLAG_COLLIDEABLE) {
    }

    if (entity->flags & ENTITY_FLAG_CHUNK_PARTITIONED) {
        u16 chunk_x = min(max(entity->position.x, 0.f), game_state->map_size.x) / game_state->chunk_size.x;
        u16 chunk_y = min(max(entity->position.z, 0.f), game_state->map_size.y) / game_state->chunk_size.y;

        if (chunk_x != entity->chunk_x || chunk_y != entity->chunk_y) {
            Chunk *chunk_old = game_state->chunks + entity->chunk_y*game_state->chunk_size.x + entity->chunk_x;

            for (Entity *it = chunk_old->first_entity, *next; it != 0; it = next) {
                next = it->next_in_chunk;

                if (it->id == entity->id) {
                    dll_remove_np(chunk_old->first_entity, chunk_old->last_entity, it, next_in_chunk, prev_in_chunk);
                    break;
                }
            }

            Chunk *chunk_new = game_state->chunks + entity->chunk_y*game_state->chunk_size.x + entity->chunk_x;
            dll_push_back_np(chunk_new->first_entity, chunk_new->last_entity, entity, next_in_chunk, prev_in_chunk);

            entity->chunk_x = chunk_x;
            entity->chunk_y = chunk_y;
        }
    }
}

internal void entity_draw(Entity *entity, f32 dt, Render_Group *render_group, Render_Commands *commands) {
    switch (entity->type) {
        default: {
            assert("INVALID DEFUALT CASE");
        } break;

        case ENTITY_TYPE_SOLDIER: {
            m4x4 transform = trs_to_transform(entity->position, entity->orientation, entity->scaling);
            if (entity->model) {
                for (u32 mesh_idx = 0; mesh_idx < entity->model->mesh_count; ++mesh_idx) {
                    Mesh *mesh = entity->model->meshes + mesh_idx;
                    push_mesh(render_group, mesh, transform, entity->animation_transform, entity->id, v2{1,1});
                }
            }

            commands->debug_transform = transform;
            commands->debug_radius = entity->radius;

            if (entity->command == ENTITY_CMD_MOVE) 
            {
                if (commands->draw_navmesh) 
                {
                    // Draw waypoints
                    //
                    for (int i = 0; i < entity->debug_waypoint_queue.count() - 1; ++i) {
                        int idx1 = ((entity->debug_waypoint_queue.front_idx + i) % array_count(entity->debug_waypoint_queue.data));
                        int idx2 = ((entity->debug_waypoint_queue.front_idx + i + 1) % array_count(entity->debug_waypoint_queue.data));
                        v3 p1 = entity->debug_waypoint_queue.data[idx1];
                        v3 p2 = entity->debug_waypoint_queue.data[idx2];
                        draw_line(render_group, p1, p2, v4{0,0,1,1});
                    }

                    // Draw portal edges.
                    //
                    for (int i = 0; i < entity->l_points.count; ++i) {
                        v2 l = entity->l_points[i];
                        v2 r = entity->r_points[i];
                        draw_line(render_group, V3(l.y, 0.2f, l.x), V3(r.y, 0.2f, r.x), v4{1,1,0,1});
                    }

                }
            }
        } break;
    }
}
