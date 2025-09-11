/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define push_entity(WORLD, ARENA, TYPE, POSITION) \
    ((TYPE *)push_entity_(WORLD, ARENA, sizeof(TYPE), POSITION))
    
internal Entity *
push_entity_(World *world, Memory_Arena *arena, umm size, v3 position) 
{
    Entity *entity      = (Entity *)push_size(arena, size);
    entity->id          = world->next_entity_id++;
    entity->position    = position;
    entity->orientation = Quaternion{1,0,0,0};
    entity->scaling     = v3{1,1,1};

    Assert(world->entity_count < arraycount(world->entities));
    world->entities[world->entity_count++] = entity;

    return entity;
}
