/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
DECLARE_ENTITY_FUNCTIONS(Rock);
struct Rock : public Entity
{
    BEGIN_ENTITY
    END_ENTITY

    void init(Game_State *game_state) {
        Entity::init();

        Game_Assets *assets = game_state->assets;
        model = assets->rock_model;

        update     = update_Rock;
        draw       = draw_Rock;
        serialize  = serialize_Rock;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Rock)
{
    Rock *e = (Rock *)entity;
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
