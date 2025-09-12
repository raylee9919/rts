/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define push_entity(WORLD, ARENA, TYPE, POSITION) \
    ((TYPE *)_push_entity(WORLD, ARENA, sizeof(TYPE), POSITION))
    
internal Entity *
_push_entity(World *world, Arena *arena, umm size, v3 position) 
{
    Entity *entity      = (Entity *)push_size(arena, size);
    entity->id          = world->next_entity_id++;
    entity->position    = position;
    entity->orientation = Quaternion{1,0,0,0};
    entity->scaling     = v3{1,1,1};

    Assert(world->entity_count < array_count(world->entities));
    world->entities[world->entity_count++] = entity;

    return entity;
}
