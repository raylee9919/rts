/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal Entity *
_push_entity(World *world, u64 size, v3 position) 
{
    Entity *entity      = (Entity *)push_size(world->arena, size);
    entity->id          = world->next_entity_id++;
    entity->position    = position;
    entity->orientation = Quaternion{1,0,0,0};
    entity->scaling     = v3{1,1,1};

    Assert(world->entity_count < array_count(world->entities));
    world->entities[world->entity_count++] = entity;

    return entity;
}
