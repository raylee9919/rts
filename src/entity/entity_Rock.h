/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
DECLARE_ENTITY_FUNCTIONS(Rock);
struct Rock : public Entity
{
    BEGIN_ENTITY
    END_ENTITY

    void init(Game_State *game_state) {
        Entity::init();

        Game_Assets *assets = game_state->game_assets;
        model = assets->rock_model;

        update     = update_Rock;
        draw       = draw_Rock;
        serialize  = serialize_Rock;
        panel      = panel_Rock;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Rock)
{
    Rock *e = (Rock *)entity;
    f32 dt = input->dt;
};

internal ENTITY_FUNCTION_DRAW(draw_Rock)
{
    Rock *e = (Rock *)entity;

    m4x4 transform = trs_to_transform(entity->position, entity->orientation, entity->scaling);
    if (e->model) {
        for (u32 mesh_idx = 0; mesh_idx < e->model->mesh_count; ++mesh_idx) {
            Mesh *mesh = e->model->meshes + mesh_idx;
            push_mesh(render_group, mesh, transform, 0, e->id, v2{1,1});
        }
    }
}

internal ENTITY_FUNCTION_PANEL(panel_Rock)
{
    Rock *e = (Rock *)entity;

    v4 color = V4(0.3f,0.0f,0.0f,0.4f);
    if (game_state->active_entity_id == entity->id) {
        entity->begin_panel(entity, game_state);
        char text[256];
        if (e->model) {
            snprintf(text, sizeof(text), "#Triangle: %u", get_triangle_count(e->model));
            ui.text(v4{0.5f,0.6f,0.3f,0.4f}, text);
        }
        entity->end_panel(entity, game_state);
    }
};
