/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
DECLARE_ENTITY_FUNCTIONS(Ground);

struct Ground : public Entity
{
    BEGIN_ENTITY
    f32 uv_scale;
    END_ENTITY

    void init(Game_State *game_state)
    {
        Entity::init();

        update      = update_Ground;
        draw        = draw_Ground;
        serialize   = serialize_Ground;

        model = game_state->game_assets->plane_model;
    }
};

internal ENTITY_FUNCTION_UPDATE(update_Ground)
{
    Ground *e = (Ground *)entity;
};

internal ENTITY_FUNCTION_DRAW(draw_Ground)
{
    Ground *e = (Ground *)entity;

    m4x4 transform = trs_to_transform(entity->position, entity->orientation, entity->scaling);
    if (e->model) {
        for (u32 mesh_idx = 0; mesh_idx < e->model->mesh_count; ++mesh_idx) {
            Mesh *mesh = e->model->meshes + mesh_idx;
            push_mesh(render_group, mesh, transform, 0, e->id, V2(e->uv_scale));
        }
    }
};
