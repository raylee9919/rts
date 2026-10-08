// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_META_H
#define RTS_META_H


#if BUILD_META
#  define REFLECT(...) __attribute__((annotate("REFLECT"#__VA_ARGS__)))
#  define FIELD(...)   __attribute__((annotate("FIELD"#__VA_ARGS__)))
#else
#  define REFLECT(...)
#  define FIELD(...)
#endif


#endif // RTS_META_H
