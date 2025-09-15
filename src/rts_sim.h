#ifndef RTS_SIM_H
#define RTS_SIM_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */




#define push_entity(WORLD, TYPE, POSITION) \
    ((TYPE *)_push_entity(WORLD, sizeof(TYPE), POSITION))
internal Entity *_push_entity(World *world, u64 size, v3 position);


#endif
