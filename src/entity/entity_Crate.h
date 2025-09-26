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

        model = game_state->assets->crate_model;
        update = update_Crate;
        draw = draw_Crate;
        serialize = serialize_Crate;

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
