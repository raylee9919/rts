/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

DECLARE_ENTITY_FUNCTIONS(Xbot);
struct Xbot : public Entity
{
    v3 velocity;
    v3 accel;
    v3 destination;

    Nav_Portal portals[256];
    u32 portal_count;
    Queue<v3> queue;

    BEGIN_ENTITY
    f32 speed;
    f32 hp;
    u32 controlled;
    END_ENTITY

    f32 transition_t;

    Animation_Channel animation_channels[1];
    m4x4 *animation_transform;
    Animation *idle_animation;
    Animation *running_animation;
    Animation *die_animation;
    Animation *attack_animation;

#if __DEVELOPER
    u32 debug_portal_edge_count = 0;
    v3 debug_portal_edges[256][2];
#endif

    void init(Game_State *game_state) {
        Entity::init();

        radius = 0.5f;

        velocity = {};
        accel = {};
        transition_t  = 0.0f;

        Game_Assets *assets = game_state->game_assets;
        model = assets->xbot_model;
        idle_animation    = assets->xbot_idle;
        running_animation = assets->xbot_run;
        die_animation     = assets->xbot_die;
        attack_animation  = assets->xbot_attack;

        animation_transform = push_array(game_state->frame_arena, m4x4, model->node_count);

        update     = update_Xbot;
        draw       = draw_Xbot;
        serialize  = serialize_Xbot;
        panel      = panel_Xbot;
    }

    void die() 
    {
        flags |= Flag_Dead;
        transition_t = 0.0f;
    }

    void move(v3 dst, f32 dt) 
    {
        v3 dir = normalize(dst - position);
        v3 forward = normalize((quaternion_to_m4x4(orientation) * v4{0,0,1,0}).xyz);
        f32 c = safe_ratio(dot(forward, dir), length(forward)*length(dir));
        if (c < 1.0f) {
            f32 radian = dt*10.0f;
            if (cross(forward, dir).y < 0.0f) {
                radian = -radian;
            }
            orientation = rotate(orientation, v3{0,1,0}, radian);
        }

        transition_t += 2.0f*dt;
        transition_t = clamp01(transition_t);
    }

    void update_position(Game_State *game_state, f32 dt) 
    {
        position = position + dt * velocity;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Xbot)
{
    Xbot *e = (Xbot *)entity;
    f32 dt = input->dt;

    // Dead?
    if (e->flags & Flag_Dead) {
        if (e->hp <= 0.0f) {
            e->die();
        }
    }

    if ((input->mouse.is_down[Mouse_Right] && input->mouse.toggle[Mouse_Right])) {
        e->command = Command_Move;

        f32 x = map01_binormal(input->mouse.position.x, 0, input->draw_dim.x);
        f32 y = map01_binormal(input->mouse.position.y, 0, input->draw_dim.y);
        m4x4 inv_view_proj = inverse(game_state->controlling_camera->VP);
        // @TODO: Graphics API-independent
        v4 up = inv_view_proj * v4{x, y, 1, 1};
        up.xyz /= up.w;
        v3 o = game_state->controlling_camera->position;
        v3 v = normalize(up.xyz - o);
        v3 n = v3{0,1,0};
        f32 d = safe_ratio(-dot(o, n), dot(v, n));
        v3 p = o + d*v;

        e->destination = p;
 
        // @TEMPORARY
        // @TODO: This is not handling 'point-on-edge' case.
        Navmesh *navmesh = &game_state->navmesh;
        Cdt_Result *cdt = &navmesh->cdt;

        v2 flatposition = v2{e->position.z, e->position.x};
        int S = -1;

        for (int i = 0; i < cdt->numtri; ++i) {
            v2 a = v2{navmesh->vertices[cdt->tri[i][0]].position.z, navmesh->vertices[cdt->tri[i][0]].position.x};
            v2 b = v2{navmesh->vertices[cdt->tri[i][1]].position.z, navmesh->vertices[cdt->tri[i][1]].position.x};
            v2 c = v2{navmesh->vertices[cdt->tri[i][2]].position.z, navmesh->vertices[cdt->tri[i][2]].position.x};
            if (cdt_point_in_triangle(flatposition, a, b, c)) {
                S = i;
                break;
            }
        }
        Assert(S != -1);

        int T = -1;
        for (int i = 0; i < cdt->numtri; ++i) {
            v2 a = v2{navmesh->vertices[cdt->tri[i][0]].position.z, navmesh->vertices[cdt->tri[i][0]].position.x};
            v2 b = v2{navmesh->vertices[cdt->tri[i][1]].position.z, navmesh->vertices[cdt->tri[i][1]].position.x};
            v2 c = v2{navmesh->vertices[cdt->tri[i][2]].position.z, navmesh->vertices[cdt->tri[i][2]].position.x};
            if (cdt_point_in_triangle(v2{p.z, p.x}, a, b, c)) {
                T = i;
                break;
            }
        }

        //
        // @TODO: A* Algorithm -> Stupid Simple Funnel Algorithm -> Add Radius (Minkowski?) -> Steering/Boid-Style Collision
        //
        if (T != -1) {
            // @NOTE: A* Algorithm
            v3 dst_cen = {};
            for (int i = 0; i < 3; ++i) {
                dst_cen += navmesh->vertices[cdt->tri[T][i]].position;
            }
            dst_cen*=0.333333f;

            f32 *euclidian_dist = (f32 *)malloc(sizeof(f32)*cdt->numtri);
            scope_exit(free(euclidian_dist));
            for (int u = 0; u < cdt->numtri; ++u) {
                v3 ucen = {};
                for (int i = 0; i < 3; ++i)
                    ucen += navmesh->vertices[cdt->tri[u][i]].position;
                ucen*=0.333333f;

                euclidian_dist[u] = distance(ucen, dst_cen);
            }

            Pair<f32, int> *dist = (Pair<f32, int> *)malloc(sizeof(Pair<f32, int>)*cdt->numtri);
            scope_exit(free(dist));
            for (int i = 0; i < cdt->numtri; ++i) {
                dist[i].x = F32_MAX;
                dist[i].y = i;
            }
            dist[S].x = euclidian_dist[S];

            Priority_Queue<Pair<f32, Pair<int, int>>> pq = {};
            enqueue(&pq, {dist[S].x, {S, -1}}); // @TODO: Is putting -1 in here correct?

            while (pq.size > 0) {
                auto item = dequeue(&pq);
                int u = item.y.x;
                int in_edge = item.y.y;

                if (u == T) {
                    break;
                }

                for (int i = 0; i < 3; ++i) {
                    int v = cdt->adj[u][i];
                    if (v != -1 && cdt->trespassable[v]) {
                        int out_edge = cdt_edge(cdt->adj, u, v);
                        f32 width;
                        if (in_edge == -1) {
                            width = F32_MAX;
                        } else {
                            width = calculate_width(navmesh, u, in_edge, out_edge);
                        }
                        if (width > 2.0f * e->radius) {
                            // @TODO: Cache centroid_distance().
                            f32 newdist = dist[u].x + centroid_distance(navmesh, u, v) + euclidian_dist[v];
                            if (dist[v].x > newdist) {
                                dist[v].x = newdist;
                                dist[v].y = u;
                                enqueue(&pq, {newdist, {v, cdt_edge(cdt->adj, v, u)}});
                            }
                        }
                    }
                }
            }


            // @NOTE: Portal list before inflating radii.
            Pair<int, int> tmp_portals[256] = {};
            umm tmp_portal_count = 0;

            for (int v = T; dist[v].y != v; v = dist[v].y) {
                int u = dist[v].y;
                int euv = cdt_edge(cdt->adj, u, v);

                Assert(tmp_portal_count < array_count(tmp_portals));
                tmp_portals[tmp_portal_count++] = {u, euv};
            }


            // @NOTE: Fill actual portal list by inflating vertices by radii.
            e->portal_count = 0;

            // @NOTE: Add src as a portal at the beginning.
            Assert(e->portal_count < array_count(e->portals));
            e->portals[e->portal_count++] = Nav_Portal{{-1, e->position}, {-1, e->position}};

            for (int portal_index = tmp_portal_count - 1; portal_index >= 0; --portal_index) {
                Pair<int, int> portal = tmp_portals[portal_index];
                int u = portal.x;
                int euv = portal.y;

                int v = cdt->adj[u][euv];
                int evu = cdt_edge(cdt->adj, v, u);

                int ai = cdt->tri[u][(euv+2)%3];
                int bi = cdt->tri[v][(evu+2)%3];
                int ci = cdt->tri[u][(euv+1)%3];
                int di = cdt->tri[u][euv];

                v3 a = navmesh->vertices[ai].position;
                v3 b = navmesh->vertices[bi].position;
                v3 c = navmesh->vertices[ci].position;
                v3 d = navmesh->vertices[di].position;

                v3 p = normalize(a - c);
                v3 q = normalize(b - c);
                v3 r = normalize(a - d);
                v3 s = normalize(b - d);
                v3 k = normalize(c - d);

                v3 h1 = normalize(p + q);
                v3 h2 = normalize(r + s);

                // @TODO: Correctness.
                if (dot(h1, k) > 0) {
                    h1 = -h1;
                }

                if (dot(h2, k) < 0) {
                    h2 = -h2;
                }

                int right_point_index = cdt->tri[u][euv];
                int left_point_index  = cdt->tri[u][(euv+1)%3];

                v3 left_point  = c + h1 * e->radius;
                v3 right_point = d + h2 * e->radius;

                Assert(e->portal_count < array_count(e->portals));
                e->portals[e->portal_count++] = Nav_Portal{{left_point_index, left_point}, {right_point_index, right_point}};
            }

            // @NOTE: Add dst as a portal at the end.
            Assert(e->portal_count < array_count(e->portals));
            e->portals[e->portal_count++] = Nav_Portal{{-1, p}, {-1, p}};


#if __DEVELOPER
            e->debug_portal_edge_count = 0;
#endif

            // @NOTE: Stupid Simple Funnel Algorithm
            clear(&e->queue);

            int funnel_apex_index  = e->portal_count - 1;
            int left_funnel_index  = e->portal_count - 1;
            int right_funnel_index = e->portal_count - 1;

            v3 funnel_apex  = e->position;
            v3 left_funnel  = e->position;
            v3 right_funnel = e->position;

            for (u32 portal_index = 0; portal_index < e->portal_count; ++portal_index) {
                Nav_Portal portal = e->portals[portal_index];

                v3 left_point  = portal.left.position;
                v3 right_point = portal.right.position;

#if __DEVELOPER
                e->debug_portal_edges[e->debug_portal_edge_count][0] = navmesh->vertices[portal.right.idx].position;
                e->debug_portal_edges[e->debug_portal_edge_count][1] = navmesh->vertices[portal.left.idx].position;
                ++e->debug_portal_edge_count;
#endif

                // Update left funnel.
                b32 left_changed = false;
                if (ssf_on_right(left_point, funnel_apex, left_funnel)) {
                    if (!ssf_equal(left_funnel, left_point)) {
                        left_changed = true;
                    }
                    left_funnel = left_point;
                    left_funnel_index = portal_index;
                }

                if (ssf_on_left(right_point, funnel_apex, right_funnel)) {
                    right_funnel = right_point;
                    right_funnel_index = portal_index;
                }

                if (!ssf_on_right(right_funnel, funnel_apex, left_funnel)) {
                    if (left_changed) {
                        funnel_apex = right_funnel;
                        funnel_apex_index = right_funnel_index;
                    } else {
                        funnel_apex = left_funnel;
                        funnel_apex_index = left_funnel_index;
                    }
                    left_funnel  = funnel_apex;
                    right_funnel = funnel_apex;

                    portal_index = funnel_apex_index;
                    left_funnel_index = funnel_apex_index;
                    right_funnel_index = funnel_apex_index;

                    enqueue(&e->queue, funnel_apex);
                }
            }

            enqueue(&e->queue, p);
        }

    }

    if (e->command == Command_Move) {
        if (!empty(&e->queue)) {
            v3 waypoint = peek(&e->queue);
            f32 dist = distance(e->position, waypoint);
            if (waypoint == e->destination) {
                // @TODO: Fix twitching character when he starts near the destination point.
                const f32 stop_radius = 1.0f;
                if (dist > stop_radius) {
                    e->move(waypoint, dt);
                } else {
                    f32 t = map01(dist, 0.0f, stop_radius);
                    e->transition_t = lerp(0.0f, t, 0.7f);
                    dequeue(&e->queue);
                }
            } else {
                const f32 waypoint_reached_radius = 0.20f;
                if (dist > waypoint_reached_radius) {
                    e->move(waypoint, dt);
                } else {
                    dequeue(&e->queue);
                }
            }
        } else {
            e->command = Command_Stop;
        }
    }

    // Animation, Position
    if (!(e->flags & Flag_Dead)) {
        f32 norm = smoothstep(e->transition_t, 0, 1);
        m4x4 rotation = quaternion_to_m4x4(e->orientation);
        f32 T = lerp(0.0f, e->transition_t, 1.0f);
        e->update_position(game_state, dt);
        e->velocity = (rotation * V4(0, 0, norm*e->speed, 0)).xyz;

        e->transition_t -= dt;
        e->transition_t = clamp01(e->transition_t);

        if (e->model) {
            f32 v = length(e->velocity);
            f32 lo = 0.0001f;
            f32 hi = 0.7f;
            Animation_Channel *channel = &e->animation_channels[0];

            if (v <= lo) {
                Animation *new_anim = e->idle_animation;
                if (channel->animation != new_anim) {
                    channel->animation = new_anim;
                    channel->dt = 0.0f;
                }
                eval(e->model, channel->animation, channel->dt, e->animation_transform, true);
                accumulate(channel, dt);
            } else if (v > hi) {
                Animation *new_anim = e->running_animation;
                if (channel->animation != new_anim) {
                    channel->animation = new_anim;
                    channel->dt = 0.0f;
                }
                eval(e->model, channel->animation, channel->dt, e->animation_transform, true);
                accumulate(channel, dt);
            } else {
                f32 t = map01(v, lo, hi);
                if (channel->animation == e->idle_animation) {
                    interpolate(e->model, channel->animation, channel->dt, t, e->running_animation, 0.0f);
                } else {
                    interpolate(e->model, e->idle_animation, 0.0f, t, channel->animation, channel->dt);
                }
                eval(e->model, 0, 0, e->animation_transform, false);
            }
        }
    } else if (e->command = Command_Dieing) {
        Animation_Channel *channel = &e->animation_channels[0];

        f32 lo = 0.0f;
        f32 hi = 0.1f;
        f32 t = map(e->transition_t, lo, hi);

        if (t < 1.0f) {
            interpolate(e->model, channel->animation, channel->dt, t, e->die_animation, 0.0f);
            eval(e->model, 0, 0, e->animation_transform, false);
        } else {
            eval(e->model, e->die_animation, e->transition_t - hi, e->animation_transform, true);
            if (e->transition_t >= e->die_animation->duration) {
                e->flags |= Flag_Dead;
            }
        }
        e->transition_t += dt;
    } else if (e->flags & Flag_Dead) {
        eval(e->model, e->die_animation, e->die_animation->duration, e->animation_transform, true);
    } else {
        INVALID_CODE_PATH;
    }
};

internal ENTITY_FUNCTION_DRAW(draw_Xbot)
{
    Xbot *e = (Xbot *)entity;

    m4x4 transform = trs_to_transform(entity->position, entity->orientation, entity->scaling);
    if (e->model) {
        for (u32 mesh_idx = 0; mesh_idx < e->model->mesh_count; ++mesh_idx) {
            Mesh *mesh = e->model->meshes + mesh_idx;
            push_mesh(render_group, mesh, transform, e->animation_transform, e->id, v2{1,1});
        }
    }

    commands->debug_transform = transform;
    commands->debug_radius = e->radius;

    if (e->command == Command_Move) {
        Queue<v3> *q = &e->queue;

        if (commands->draw_navmesh) {
            for (size_t i = q->front;
                 i < (q->front + q->count) % array_count(q->data);
                 i = (i+1)%array_count(q->data)) {

                //push_mesh(render_group, game_state->game_assets->sphere_model->meshes, scale(translate(identity(), q->data[i]), V3(0.2f)), 0, 0, v2{1,1});

                if (i != (q->front + q->count - 1) % array_count(q->data)) {
                    draw_line(render_group, q->data[i], q->data[(i+1)%array_count(q->data)], v4{1,0,1,1});
                }
            }

            if (!empty(q)) {
                draw_line(render_group, e->position, q->data[q->front], v4{1,0,1,1});
            }

            if (e->debug_portal_edge_count > 0) {
                for (umm i = 0; i < e->debug_portal_edge_count; ++i) {
                    draw_line(render_group, e->debug_portal_edges[i][0], e->debug_portal_edges[i][1], v4{0,0,1,1});
                }
            }
        }
    }
}

internal ENTITY_FUNCTION_PANEL(panel_Xbot)
{
    Xbot *e = (Xbot *)entity;

    v4 color = V4(0.3f,0.0f,0.0f,0.4f);
    if (game_state->active_entity_id == entity->id) {
        entity->begin_panel(entity, game_state);
        char text[256];
        if (e->model) {
            snprintf(text, sizeof(text), "#Triangle: %u", get_triangle_count(e->model));
            ui.text(v4{0.5f,0.6f,0.3f,0.4f}, text);
        }
        ui.slider(&e->radius, 0.0f, 1.0f, color, "Radius", 0);
        entity->end_panel(entity, game_state);
    }
};
