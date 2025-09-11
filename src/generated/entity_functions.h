internal ENTITY_FUNCTION_SERIALIZE(serialize_Camera) 
{
    if (entity->id != 0) {

        Camera *e = (Camera *)entity;

        fprintf(file, "\nCamera\n");
        fprintf(file, "1\n");
        fprintf(file, "%u\n", e->id);

        e->serialize_entity(entity, game_state, file);
        fprintf(file, R"entity(; type
    %u
)entity", e->type);

        fprintf(file, R"ENTITY(; focal_length
    %.6f : 0x%X
)ENTITY", e->focal_length, f32_to_raw(e->focal_length));

        fprintf(file, R"ENTITY(; width
    %.6f : 0x%X
)ENTITY", e->width, f32_to_raw(e->width));

        fprintf(file, R"ENTITY(; height
    %.6f : 0x%X
)ENTITY", e->height, f32_to_raw(e->height));

        fprintf(file, R"ENTITY(; N
    %.6f : 0x%X
)ENTITY", e->N, f32_to_raw(e->N));

        fprintf(file, R"ENTITY(; F
    %.6f : 0x%X
)ENTITY", e->F, f32_to_raw(e->F));

    }
} // Camera

internal ENTITY_FUNCTION_SERIALIZE(serialize_Crate) 
{
    if (entity->id != 0) {

        Crate *e = (Crate *)entity;

        fprintf(file, "\nCrate\n");
        fprintf(file, "1\n");
        fprintf(file, "%u\n", e->id);

        e->serialize_entity(entity, game_state, file);
    }
} // Crate

internal ENTITY_FUNCTION_SERIALIZE(serialize_Entity) 
{
    if (entity->id != 0) {

        Entity *e = (Entity *)entity;

        fprintf(file, R"ENTITY(; position
    %.6f : 0x%X
    %.6f : 0x%X
    %.6f : 0x%X
)ENTITY", e->position.x, f32_to_raw(e->position.x), e->position.y, f32_to_raw(e->position.y), e->position.z, f32_to_raw(e->position.z));

        fprintf(file, R"ENTITY(; orientation
    %.6f : 0x%X
    %.6f : 0x%X
    %.6f : 0x%X
    %.6f : 0x%X
)ENTITY", e->orientation.w, f32_to_raw(e->orientation.w), e->orientation.x, f32_to_raw(e->orientation.x), e->orientation.y, f32_to_raw(e->orientation.y), e->orientation.z, f32_to_raw(e->orientation.z));

        fprintf(file, R"ENTITY(; scaling
    %.6f : 0x%X
    %.6f : 0x%X
    %.6f : 0x%X
)ENTITY", e->scaling.x, f32_to_raw(e->scaling.x), e->scaling.y, f32_to_raw(e->scaling.y), e->scaling.z, f32_to_raw(e->scaling.z));

    }
} // Entity

internal ENTITY_FUNCTION_SERIALIZE(serialize_Xbot) 
{
    if (entity->id != 0) {

        Xbot *e = (Xbot *)entity;

        fprintf(file, "\nXbot\n");
        fprintf(file, "1\n");
        fprintf(file, "%u\n", e->id);

        e->serialize_entity(entity, game_state, file);
        fprintf(file, R"ENTITY(; u
    %.6f : 0x%X
)ENTITY", e->u, f32_to_raw(e->u));

        fprintf(file, R"ENTITY(; hp
    %.6f : 0x%X
)ENTITY", e->hp, f32_to_raw(e->hp));

        fprintf(file, R"entity(; controlled
    %u
)entity", e->controlled);

    }
} // Xbot

