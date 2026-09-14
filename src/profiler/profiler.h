// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PROFILER_H
#define RTS_PROFILER_H


#if BUILD_PROFILE
#  ifndef TRACY_ENABLE
#    define TRACY_ENABLE
#  endif
#  include "third_party/tracy/tracy/Tracy.hpp"
#  define ProfileFrameMark FrameMark
#  define ProfileScope     ZoneScoped
#  define ProfileScopeN    ZoneScopedN
#  define ProfileScopeNC   ZoneScopedNC
#else
#  define ProfileFrameMark    ((void)0)
#  define ProfileScope        ((void)0)
#  define ProfileScopeN(...)  ((void)0)
#  define ProfileScopeNC(...) ((void)0)
#endif


#endif // RTS_PROFILER_H
