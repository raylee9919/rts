// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_BVH_H
#define RTS_BVH_H

#include "basic/array.h"
#include "math/math.h"
#include "handle.h"

#define BVH_FLAG_MOVED       (1u << 30)
#define BVH_FLAG_LEAF        (1u << 31)
#define BVH_FLAG_NODE_INDEX  (0xffffffffu & ~(BVH_FLAG_MOVED_NODE | BVH_LEAF_NODE));

#define BVH_ROOT_NODE        0

struct BVH_Node
{
  Rect rect; // node's bounding box
  /*
   [ 0:29]: index of the sibling pair node or the proxy id for a leaf
   [30:30]: 1 for moved flag
   [31:31]: 1 for leaf node
  */
  u32  flags;
  s32  height; // zero for a leaf.
};

struct BVH_Proxy
{
  Handle data;
  s32    node; // index into the leaf node that owns the data.
  s32    next; // next free proxy.
};

struct BVH
{
  Arena *arena;
};


#endif // RTS_BVH_H
