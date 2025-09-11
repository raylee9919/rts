/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */


DECLARE_ENTITY_FUNCTIONS(Camera);

enum Camera_Type {
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

        Assert(world->camera_count < arraycount(world->cameras));
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

    Game_Key *keys = input->keys;
    Mouse_Input mouse = input->mouse;

    if (game_state->controlling_camera == camera) {
        if (camera->following) {
            camera->position = camera->following->position + v3{0.0f, 3.0f, 3.0f};
        } else if (!input->interacted_ui) {
            f32 C = input->dt * 10.0f;
            if (keys[KEY_W].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(0, 0, -C, 0)).xyz;
            }
            if (keys[KEY_S].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(0, 0, C, 0)).xyz;
            }
            if (keys[KEY_D].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(C, 0, 0, 0)).xyz;
            }
            if (keys[KEY_A].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(-C, 0, 0, 0)).xyz;
            }

            if (keys[KEY_Q].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(0, -C, 0, 0)).xyz;
            }
            if (keys[KEY_E].is_down) {
                m4x4 rotation = quaternion_to_m4x4(camera->orientation);
                camera->position += (rotation * V4(0, C, 0, 0)).xyz;
            }

            if (mouse.is_down[Mouse_Left]) {
                if (mouse.toggle[Mouse_Left]) {
                    input->prev_mouse_p = mouse.click_p[Mouse_Left];
                } else {
                    v2 d = 0.5f * input->dt * (mouse.position - input->prev_mouse_p);
                    camera->orientation = build_quaternion(v3{0,1,0}, -d.x) * camera->orientation;
                    camera->orientation = build_quaternion((quaternion_to_m4x4(camera->orientation)*v4{1,0,0,0}).xyz, d.y) * camera->orientation;
                }
            }
        }
    }

    if (camera->type == Camera_Type_Perspective) {
        camera->width = 2.0f;
        f32 h_over_w = (f32)input->draw_dim.h / (f32)input->draw_dim.w;
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
    } else if (camera->type == Camera_Type_Orthographic) {
        camera->width  = (f32)input->draw_dim.w;
        camera->height = (f32)input->draw_dim.h;

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
    } else {
        INVALID_CODE_PATH;
    }
};

internal ENTITY_FUNCTION_DRAW(draw_Camera)
{
};
