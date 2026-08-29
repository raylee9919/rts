// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PROFILER_H
#define RTS_PROFILER_H


#if BUILD_PROFILE
#  define TRACY_ENABLE
#  include "third_party/tracy/tracy/Tracy.hpp"
#  include "third_party/tracy/TracyClient.cpp"
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


#endif // RTS_PROFILER_H
