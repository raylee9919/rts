/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Entity;
struct Render_Group;

#define ENTITY_FUNCTION_UPDATE(NAME) void NAME(Entity *entity, Game_State *game_state, Input *input)
typedef ENTITY_FUNCTION_UPDATE(Entity_Function_Update);

#define ENTITY_FUNCTION_DRAW(NAME) void NAME(Entity *entity, Game_State *game_state, Render_Commands *commands, Render_Group *render_group, Render_Group *orthographic_group)
typedef ENTITY_FUNCTION_DRAW(Entity_Function_Draw);

#define ENTITY_FUNCTION_SERIALIZE(NAME) void NAME(Entity *entity, Game_State *game_state, FILE *file)
typedef ENTITY_FUNCTION_SERIALIZE(Entity_Function_Serialize);

#define ENTITY_FUNCTION_PANEL(NAME) void NAME(Entity *entity, Game_State *game_state)
typedef ENTITY_FUNCTION_PANEL(Entity_Function_Panel);

#define DECLARE_ENTITY_FUNCTIONS(ENTITY)\
    internal ENTITY_FUNCTION_UPDATE(update_##ENTITY);\
internal ENTITY_FUNCTION_DRAW(draw_##ENTITY);\
internal ENTITY_FUNCTION_SERIALIZE(serialize_##ENTITY);\
internal ENTITY_FUNCTION_PANEL(panel_##ENTITY);

internal ENTITY_FUNCTION_SERIALIZE(serialize_Entity);


enum Entity_Command {
    Command_Invalid = 0x0,
    Command_Stop,
    Command_Move,
    Command_Dieing,
};

enum Entity_Flags : u64 {
    Flag_Dead       = (1<<0),
    Flag_Navmesh    = (1<<1),
};

struct Entity 
{
    u32 id;

    Model *model;

    BEGIN_ENTITY
    v3 position;
    Quaternion orientation;
    v3 scaling;
    f32 radius;
    END_ENTITY

    Entity_Command command;
    u64 flags;

    Entity_Function_Update      *update;
    Entity_Function_Draw        *draw;

    Entity_Function_Serialize   *serialize;
    Entity_Function_Serialize   *serialize_entity;

    Entity_Function_Panel       *panel;

    void init() {
        serialize_entity = serialize_Entity;
    }

    void destroy() {
        id = 0;
    }

    void begin_panel(Entity *entity, Game_State *game_state) {
        v4 color = V4(0.3f,0.0f,0.0f,0.4f);
        char text[256];
        snprintf(text, sizeof(text), "Entity ID#%u", entity->id);

        ui.begin("Entity Panel", V2(0.5f,0.5f));
        ui.text(color, text);

        ui.gizmo(&entity->position, game_state->view_proj);

        ui.slider(&entity->position.x, -20.0f, 20.0f, color, "position x");
        ui.slider(&entity->position.y, -20.0f, 20.0f, color, "position y");
        ui.slider(&entity->position.z, -20.0f, 20.0f, color, "position z");

        ui.slider(&entity->scaling.x, 0.000001f, 20.0f, color, "scale x");
        ui.slider(&entity->scaling.y, 0.000001f, 20.0f, color, "scale y");
        ui.slider(&entity->scaling.z, 0.000001f, 20.0f, color, "scale z");
    }

    void end_panel(Entity *entity, Game_State *game_state) {
        v4 color = v4{0.3f,0.0f,0.0f,0.4f};

        if (ui.button(V4(V3(0.5f, 0.1f, 0.5f), 0.4f), "Destroy!")) {
            destroy();
        }
        ui.end();
    }
};
