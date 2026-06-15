// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


#if BUILD_PROFILE
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
