#ifndef EMBED_PROFILE_H
#define EMBED_PROFILE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#if BUILD_DEBUG
#  define TRACY_ENABLE
#  include "tracy/tracy/Tracy.hpp"
#  include "tracy/TracyClient.cpp"
#  define ProfileFrameMark FrameMark
#  define ProfileScope     ZoneScoped
#  define ProfileScopeN    ZoneScopedN
#  define ProfileScopeNC   ZoneScopedNC
#else
#  define ProfileFrameMark
#  define ProfileScope
#  define ProfileScopeN
#  define ProfileScopeNC
#endif




#endif // EMBED_PROFILE_H
