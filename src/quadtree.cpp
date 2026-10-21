// Copyright Seong Woo Lee. All Rights Reserved.

#include "./quadtree.h"
#include "game.h"
#include "renderer/renderer.h"
#include "renderer/immediate.h"

static void attach_to_leaf(Quadtree *tree, Quadtree_Node *node, Quadtree_Item item);

static b32
contains_(Rect out, Rect in)
{
  return (in.min.x >= out.min.x && in.min.y >= out.min.y &&
          in.max.x <= out.max.x && in.max.y <= out.max.y);
}

static b32
is_leaf_(Quadtree_Node *node)
{
  return node->first_child_index == -1;
}

static Quadtree_Node *
new_node_(Quadtree *tree, s32 depth, Rect rect)
{
  append(&tree->nodes, {});
  s32 index = tree->nodes.count - 1;
  Quadtree_Node *node = &tree->nodes.data[index];
  {
    node->index = index;
    node->first_child_index = -1;
    node->depth = depth;
    node->num_items = 0;
    node->item_index = -1;
    node->rect = rect;
  }
  return node;
}

static void
append_(Quadtree *tree, Quadtree_Node *node, Quadtree_Item item)
{
  append(&tree->items, item);
  node->item_index = tree->items.count - 1;
}

static void 
split_(Quadtree *tree, Quadtree_Node *node)
{
  node->first_child_index = tree->nodes.count;
  for (int i = 0; i < 4; ++i) {
    f32 hx = (node->rect.min.x + node->rect.max.x) * 0.5f;
    f32 hy = (node->rect.min.y + node->rect.max.y) * 0.5f;
    vec2 mins[4] = { node->rect.min, vec2{hx, node->rect.min.y}, vec2{node->rect.min.x, hy}, vec2{hx, hy} };
    vec2 maxs[4] = { vec2{hx, hy}, vec2{node->rect.max.x, hy}, vec2{hx, node->rect.max.y}, node->rect.max };
    Rect rect = { mins[i], maxs[i] };
    new_node_(tree, node->depth + 1, rect);
  }

  Array<s32> items(tctx.temp);
  for (s32 i = 0; i < node->num_items; ++i) {
  }

  node->num_items = 0;

  for (s32 i = 0; i < num; ++i) {
    attach_to_leaf(tree, node, items[i]);
  }
}

static s32 
child_enclosing_rect_(Quadtree *tree, Quadtree_Node *node, Rect r)
{
  if (is_leaf_(node)) return -1;

  for (s32 _ = 0; _ < 4; ++_) {
    s32 index = node->first_child_index + _;
    Quadtree_Node *child = &tree->nodes.data[index];
    if (contains_(child->rect, r)) {
      return index;
    }
  }
  return -1;
}

static void 
attach_to_leaf(Quadtree *tree, Quadtree_Node *node, Handle item, Rect rect)
{
  // Premise: node encloses the rect.

  // I'm not a leaf. Which of my child should I recurse into?
  s32 child_index = child_enclosing_rect_(tree, node, rect);
  if (child_index >= 0) {
    attach_to_leaf(tree, tree->nodes.data + child_index, item, rect);
    return;
  }

  // I'm a leaf or the rect stradles across splits. 
  // Item belongs to myself.
  append_(tree, node, { item, rect });

  // Only split leaves.
  if (is_leaf_(node) && 
      node->num_items > QUADTREE_ITEM_THRESHOLD && 
      node->depth < QUADTREE_MAX_DEPTH) {
    split_(tree, node);
  }
}

static void
dfs_(Game_State *g, Entity *e, u64 i, void *user_data)
{
  if (e->radius <= 0.f) return;
  Quadtree *tree = (Quadtree *)user_data;
  Quadtree_Node *root = &tree->nodes.data[0];
  vec2 r = vec2(e->radius);
  Rect rect = { e->position.xy - r, e->position.xy + r };
  attach_to_leaf(tree, root, handle_from_entity(g, e), rect);
}

void
quadtree_build(Game_State  *g, 
               Quadtree    *tree, 
               vec2         min, 
               vec2         max)
{
  // @Todo: allocator
  array_reset_keeping_memory(&tree->nodes);
  tree->nodes.allocator = tctx.temp;
  new_node_(tree, 0, Rect{min,max});
  entity_dfs(g, g->root, tree, dfs_);
}



//
// Internal purpose.
//
void 
quadtree_visualize(Quadtree *tree, Camera *camera)
{
  vec4 c = vec4(1,1,1,1);
  for (u32 i = 0; i < tree->nodes.count; ++i) {
    Quadtree_Node *node = tree->nodes.data + i;
    vec4 clip_00 = camera_project(camera, vec3(node->rect.min, 1.f));
    vec4 clip_11 = camera_project(camera, vec3(node->rect.max, 1.f));
    vec4 clip_01 = camera_project(camera, vec3(node->rect.max.x, node->rect.min.y, 1.f));
    vec4 clip_10 = camera_project(camera, vec3(node->rect.min.x, node->rect.max.y, 1.f));
    vec2 _00 = vec2(RESOLUTION_X, RESOLUTION_Y) * (vec2(0.5f, -0.5f) * (clip_00.xy / clip_00.w) + vec2(0.5f));
    vec2 _11 = vec2(RESOLUTION_X, RESOLUTION_Y) * (vec2(0.5f, -0.5f) * (clip_11.xy / clip_11.w) + vec2(0.5f));
    vec2 _01 = vec2(RESOLUTION_X, RESOLUTION_Y) * (vec2(0.5f, -0.5f) * (clip_01.xy / clip_01.w) + vec2(0.5f));
    vec2 _10 = vec2(RESOLUTION_X, RESOLUTION_Y) * (vec2(0.5f, -0.5f) * (clip_10.xy / clip_10.w) + vec2(0.5f));
    if (clip_00.w >= 0.05f) {
      if (clip_01.w >= 0.05f) {
        draw_line(_00, _01, c, 1.f);
      }
      if (clip_10.w >= 0.05f) {
        draw_line(_00, _10, c, 1.f);
      }
    }
    if (clip_11.w >= 0.05f) {
      if (clip_01.w >= 0.05f) {
        draw_line(_11, _01, c, 1.f);
      }
      if (clip_10.w >= 0.05f) {
        draw_line(_11, _10, c, 1.f);
      }
    }
  }
}
