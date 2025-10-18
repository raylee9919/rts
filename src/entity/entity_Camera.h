/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


DECLARE_ENTITY_FUNCTIONS(Camera);

enum Camera_Type 
{
    Camera_Type_Perspective,
    Camera_Type_Orthographic
};

struct Camera : public Entity
{
    BEGIN_ENTITY
    u8 type;
    f32 focal_length;
    f32 width;
    f32 height;
    f32 N;
    f32 F;
    END_ENTITY

    v3 velocity;
    v3 accel;

    m4x4 V;
    m4x4 P;
    m4x4 VP;

    Entity *following;

    // @Temporary
    void init(Game_State *game_state) {
    }

    void init(Camera_Type _type, f32 _focal_length, f32 _N, f32 _F, World *world, Entity *_following = 0) {
        Entity::init();

        type         = _type;
        focal_length = _focal_length;
        N            = _N;
        F            = _F;
        following    = _following;

        assert(world->camera_count < array_count(world->cameras));
        world->cameras[world->camera_count++] = this;

        update      = update_Camera;
        draw        = draw_Camera;
        //serialize   = serialize_Camera;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Camera)
{
    Camera *camera = (Camera *)entity;

    camera->width  = 1920;
    camera->height = 1080;

    if (game_state->controlling_camera == camera) 
    {
        if (camera->following) 
        {
            camera->position = camera->following->position + v3{0.0f, 3.0f, 3.0f};
        }

        f32 dt              = game_state->dt_game;
        f32 accel_strength  = 50.0f;
        f32 friction        = 7.0f;
        f32 max_speed       = 20.0f;
        m4x4 rotation       = quaternion_to_m4x4(camera->orientation);
        v3 desired_dir      = {};

        {
            desired_dir += os->key_is_down[OS_KEY_W] ? (rotation * V4( 0,  0, -1, 0)).xyz : v3{};
            desired_dir += os->key_is_down[OS_KEY_A] ? (rotation * V4(-1,  0,  0, 0)).xyz : v3{};
            desired_dir += os->key_is_down[OS_KEY_S] ? (rotation * V4( 0,  0,  1, 0)).xyz : v3{};
            desired_dir += os->key_is_down[OS_KEY_D] ? (rotation * V4( 1,  0,  0, 0)).xyz : v3{};
            desired_dir += os->key_is_down[OS_KEY_Q] ? (rotation * V4( 0, -1,  0, 0)).xyz : v3{};
            desired_dir += os->key_is_down[OS_KEY_E] ? (rotation * V4( 0,  1,  0, 0)).xyz : v3{};

            if (os->key_is_down[OS_KEY_SHIFT])
            {
                accel_strength *= 2.f;
                max_speed *= 2.f;
            }
        }

        if (sqlen(desired_dir) > 0.0f)
        {
            desired_dir = normalize(desired_dir); 
        }

        v3 target_accel = desired_dir * accel_strength;
        camera->velocity += (dt*target_accel);

        if (sqlen(desired_dir) == 0.0f)
        {
            camera->velocity -= camera->velocity * friction * dt; 
        }

        f32 speed = length(camera->velocity);
        if (speed > max_speed) 
        {
            camera->velocity = (camera->velocity / speed) * max_speed; 
        }

        camera->position += (dt*camera->velocity); 

        // @Hack:
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
                camera->orientation = build_quaternion(v3{0,1,0}, -d.x) * camera->orientation;
                camera->orientation = build_quaternion((quaternion_to_m4x4(camera->orientation)*v4{1,0,0,0}).xyz, -d.y) * camera->orientation;
                mouse_position_last = os->mouse_position_last;
            }

            if (event->type == OS_EVENT_RELEASE && event->key == OS_KEY_MOUSE_LEFT)
            {
                os_event_consume(event);
                dragging = false;
            }
        }
    }

    if (camera->type == Camera_Type_Perspective) 
    {
        camera->width = 2.0f;
        f32 h_over_w = (f32)game_state->draw_height / (f32)game_state->draw_width;
        camera->height = camera->width * h_over_w;

        m4x4 V = view_transform(camera->position, camera->orientation);
        f32 f = camera->focal_length;
        f32 N = camera->N;
        f32 F = camera->F;
        f32 a = safe_ratio(2.0f * f, camera->width);
        f32 b = safe_ratio(2.0f * f, camera->height);
        f32 c = (N + F) / (N - F);
        f32 d = (2 * N * F) / (N - F);
        m4x4 P = {{
            { a,  0,  0,  0},
            { 0,  b,  0,  0},
            { 0,  0,  c,  d},
            { 0,  0, -1,  0}
        }};
        camera->V = V;
        camera->P = P;
        camera->VP = P*V;
    } 
    else if (camera->type == Camera_Type_Orthographic) 
    {
        camera->width  = (f32)game_state->draw_width;
        camera->height = (f32)game_state->draw_height;

        m4x4 camera_rotation = quaternion_to_m4x4(camera->orientation);
        m4x4 V = camera_transform(get_column(camera_rotation, 0),
                                  get_column(camera_rotation, 1),
                                  get_column(camera_rotation, 2),
                                  camera->position);

        f32 w = safe_ratio(2.0f, camera->width);
        f32 h = safe_ratio(2.0f, camera->height);
        f32 N = camera->N;
        f32 F = camera->F;
        f32 a = safe_ratio(2.0f, N-F);
        f32 b = safe_ratio(F+N, F-N);
        m4x4 P = m4x4{{
            { w,  0,  0, -1},
            { 0,  h,  0, -1},
            { 0,  0,  a,  b},
            { 0,  0,  0,  1}
        }};

        camera->VP = P*V;
    } 
    else 
    {
        INVALID_CODE_PATH;
    }
};

internal ENTITY_FUNCTION_DRAW(draw_Camera)
{
};
