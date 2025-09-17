/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
DECLARE_ENTITY_FUNCTIONS(Crate);
struct Crate : public Entity
{
    void init(Game_State *game_state) {
        Entity::init();

        model = game_state->game_assets->crate_model;
        update = update_Crate;
        draw = draw_Crate;
        serialize = serialize_Crate;
        panel = panel_Crate;

        flags |= Flag_Navmesh;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Crate)
{
    Crate *e = (Crate *)entity;
};

internal ENTITY_FUNCTION_DRAW(draw_Crate)
{
    Crate *crate = (Crate *)entity;

    m4x4 transform = trs_to_transform(entity->position, entity->orientation, entity->scaling);
    if (crate->model) {
        for (u32 mesh_idx = 0; mesh_idx < crate->model->mesh_count; ++mesh_idx) {
            Mesh *mesh = crate->model->meshes + mesh_idx;
            push_mesh(render_group, mesh, transform, 0, crate->id, v2{1,1});
        }
    }
}

internal ENTITY_FUNCTION_PANEL(panel_Crate)
{
    Crate *e = (Crate *)entity;

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
}
