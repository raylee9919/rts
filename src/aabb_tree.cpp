// Copyright Seong Woo Lee. All Rights Reserved.

#include "./aabb_tree.h"
#include "renderer/immediate.h"
#include "game.h"

// @Todo: The code is using surface area of a 3D box at the moment. 
//        My game is 3D but the simulation mostly works on 2D, so 
//        I might want to use 2D SAH instead of 3D. Sounds like a 
//        fun thing to test.

#define AABB_TREE_INITIAL_NODE_COUNT  64
#define AABB_TREE_INITIAL_PROXY_COUNT 64
#define AABB_TREE_STACK_SIZE          512 // @Robustness

force_inline b32 
is_leaf_(Box_Node *node)
{
  return node->left == -1 && node->right == -1;
}

static void 
swap_nodes_(Box_Tree *tree, s32 index1, s32 index2) 
{
  Box_Node *nodes = tree->nodes;
  s32 parent1_i = nodes[index1].parent;
  s32 parent2_i = nodes[index2].parent;
  Box_Node *parent1 = &nodes[parent1_i];
  Box_Node *parent2 = &nodes[parent2_i];

  nodes[index1].parent = parent2_i;
  nodes[index2].parent = parent1_i;

  if (parent1->left == index1) {
    parent1->left = index2;
  } else {
    parent1->right = index2;
  }

  if (parent2->left == index2) {
    parent2->left = index1;
  } else {
    parent2->right = index1;
  }
}

void 
aabb_tree_init(Box_Tree *tree, Arena *arena)
{
  tree->arena = arena;

  tree->cap_nodes = AABB_TREE_INITIAL_NODE_COUNT;
  tree->free_node = 0;
  tree->nodes     = push_array(arena, Box_Node, tree->cap_nodes);
  tree->num_nodes = 0;
  for (s32 i = 0; i < tree->cap_nodes-1; ++i) {
    tree->nodes[i].next = i+1;
  }
  tree->nodes[tree->cap_nodes-1].next = -1;

  tree->num_proxies = 0;
  tree->cap_proxies = AABB_TREE_INITIAL_PROXY_COUNT;
  tree->proxies     = push_array(arena, Box_Proxy, tree->cap_proxies);
  tree->free_proxy  = 0;
  for (s32 i = 0; i < tree->cap_proxies-1; ++i) {
    tree->proxies[i].next = i+1;
  }
  tree->proxies[tree->cap_proxies-1].next = -1;

  tree->root = -1;
}

static s32 
proxy_alloc_(Box_Tree *tree, Handle item)
{
  if (tree->num_proxies >= tree->cap_proxies)
  {
    s32 new_cap = tree->cap_proxies + (tree->cap_proxies >> 1);
    Box_Proxy *new_proxies = push_array(tree->arena, Box_Proxy, new_cap);
    memcpy(new_proxies, tree->proxies, sizeof(Box_Proxy) * tree->cap_proxies);
    tree->proxies = new_proxies;
    for (s32 i = tree->cap_proxies; i < new_cap-1; ++i) 
    {
      tree->proxies[i].next = i + 1;
    }
    tree->proxies[new_cap-1].next = -1;
    tree->free_proxy = tree->cap_proxies;
    tree->cap_proxies = new_cap;
  }

  s32 free = tree->free_proxy;
  R_ASSERT(free != -1);
  tree->free_proxy = tree->proxies[free].next;
  tree->proxies[free].item = item;
  tree->num_proxies += 1;
  return free;
}

static void
proxy_dealloc_(Box_Tree *tree, s32 proxy_index)
{
  R_ASSERT(proxy_index < tree->cap_proxies);
  Box_Proxy *proxy = &tree->proxies[proxy_index];
  proxy->next = tree->free_proxy;
  tree->free_proxy = proxy_index;
  tree->num_proxies -= 1;
}

static s32 
node_alloc_(Box_Tree *tree)
{
  if (tree->num_nodes >= tree->cap_nodes)
  {
    s32 new_cap = tree->cap_nodes + (tree->cap_nodes >> 1);
    Box_Node *new_nodes = push_array(tree->arena, Box_Node, new_cap);
    memcpy(new_nodes, tree->nodes, sizeof(Box_Node) * tree->cap_nodes);
    tree->nodes = new_nodes;
    for (s32 i = tree->cap_nodes; i < new_cap-1; ++i) {
      tree->nodes[i].next = i + 1;
    }
    tree->nodes[new_cap-1].next = -1;
    tree->free_node = tree->cap_nodes;
    tree->cap_nodes = new_cap;
  }

  s32 free = tree->free_node;
  R_ASSERT(free != -1);
  tree->free_node = tree->nodes[free].next;
  tree->num_nodes += 1;
  return free;
}

static void
node_dealloc_(Box_Tree *tree, s32 node_index)
{
  R_ASSERT(node_index < tree->cap_nodes);
  Box_Node *node = &tree->nodes[node_index];
  node->next = tree->free_node;
  tree->free_node = node_index;
  tree->num_nodes -= 1;
}

// @Speed: Don't compute every time. Pass parent's to the children.
static f32
compute_inherited_cost(Box_Tree *tree, Box_Node *node, AABBV box)
{
  f32 cost = 0.f;
  for (;;)
  {
    if (node->parent == -1)
    {
      break;
    }
    node = &tree->nodes[node->parent];
    f32 delta = aabbv_surface(aabbv_union(node->box, box)) - aabbv_surface(node->box);
    cost += delta;
  }
  return cost;
}

static s32 
find_best_sibling_(Box_Tree *tree, AABBV box)
{
  R_ASSERT(tree->root != -1);

  f32 best    = INFINITY;
  s32 sibling = -1;

  s32 stack[AABB_TREE_STACK_SIZE];
  s32 top = -1;

  stack[++top] = tree->root;

  while (top >= 0)
  {
    s32 index = stack[top--];
    Box_Node *node = &tree->nodes[index];

    f32 direct    = aabbv_surface(aabbv_union(node->box, box));
    f32 inherited = compute_inherited_cost(tree, node, box);
    f32 total     = direct + inherited;

    if (total < best)
    {
      best    = total;
      sibling = index;
    }

    // Branch and Bound
    f32 lower_bound = aabbv_surface(box) + inherited + (direct - aabbv_surface(node->box));
    if (lower_bound < best)
    {
      R_ASSERT(top + 2 < AABB_TREE_STACK_SIZE);
      if (node->left != -1)
      {
        stack[++top] = node->left;
      }

      if (node->right != -1)
      {
        stack[++top] = node->right;
      }
    }
  }

  R_ASSERT(sibling != -1);
  return sibling;
}

static void
refit_from_(Box_Tree *tree, Box_Node *start)
{
  Box_Node *node = start;
  for (;;)
  {
    if (node->parent == -1)
    {
      break;
    }
    node = &tree->nodes[node->parent];
    node->box = aabbv_union(tree->nodes[node->left].box, 
                            tree->nodes[node->right].box);
  }
}


/* 
         A
       /   \
      B     C
     / \   / \
    D   E F   G  
*/ 
static void
rotate_(Box_Tree *tree, s32 index)
{
  R_ASSERT(index != -1);
  Box_Node *A = &tree->nodes[index];
  s32 iB = A->left;
  s32 iC = A->right;
  R_ASSERT(iB != -1);
  R_ASSERT(iC != -1);
  Box_Node *B = &tree->nodes[iB];
  Box_Node *C = &tree->nodes[iC];

  b32 leafB = is_leaf_(B);
  b32 leafC = is_leaf_(C);

  if (leafB && leafC)
  {
    return;
  }

  f32 surfaceB = aabbv_surface(tree->nodes[iB].box);
  f32 surfaceC = aabbv_surface(tree->nodes[iC].box);

  f32 best  = 0.f;
  s32 iUp   = -1;
  s32 iDown = -1;

  /* 
               A            A     
             /   \        /   \            
           [B]    C     (F)    C 
                 / \          / \            
               (F)  G       [B]  G    ? DeltaC
  */ 
  if (!leafC)
  {
    s32 iF = tree->nodes[iC].left;
    s32 iG = tree->nodes[iC].right;

    f32 deltaBF = aabbv_surface(aabbv_union(tree->nodes[iB].box, tree->nodes[iG].box)) - surfaceC;
    if (deltaBF < best)
    {
      best  = deltaBF;
      iUp   = iB;
      iDown = iF;
    }

    f32 deltaBG = aabbv_surface(aabbv_union(tree->nodes[iB].box, tree->nodes[iF].box)) - surfaceC;
    if (deltaBG < best)
    {
      best  = deltaBG;
      iUp   = iB;
      iDown = iG;
    }
  }

  /* 
               A            A     
             /   \        /   \            
            B    [C]     B    (F)
           / \          / \                    
         (F)  G       [C]  G          ? DeltaB
  */ 
  if (!leafB)
  {
    s32 iF = tree->nodes[iB].left;
    s32 iG = tree->nodes[iB].right;

    f32 deltaCF = aabbv_surface(aabbv_union(tree->nodes[iC].box, tree->nodes[iG].box)) - surfaceB;
    if (deltaCF < best)
    {
      best  = deltaCF;
      iUp   = iC;
      iDown = iF;
    }

    f32 deltaCG = aabbv_surface(aabbv_union(tree->nodes[iC].box, tree->nodes[iF].box)) - surfaceB;
    if (deltaCG < best)
    {
      best  = deltaCG;
      iUp   = iC;
      iDown = iG;
    }
  }

  if (iUp != -1)
  {
    swap_nodes_(tree, iUp, iDown);
    s32 iMid = tree->nodes[iUp].parent;
    tree->nodes[iMid].box = aabbv_union(tree->nodes[tree->nodes[iMid].left].box, 
                                        tree->nodes[tree->nodes[iMid].right].box);
  }
}

// Returns the index of the inserted proxy to the user.
// Yes, it acts as a handle.
s32 
aabb_tree_insert(Box_Tree *tree, Handle item, AABBV box)
{
  s32 proxy_index = proxy_alloc_(tree, item);
  Box_Proxy *proxy = &tree->proxies[proxy_index];

  s32 leaf = node_alloc_(tree);
  {
    tree->nodes[leaf].box   = box;
    tree->nodes[leaf].left  = -1;
    tree->nodes[leaf].right = -1;
    tree->nodes[leaf].proxy = proxy_index;
  }

  proxy->node = leaf;

  if (tree->root == -1)
  {
    tree->nodes[leaf].parent = -1;
    tree->root = leaf;
    return proxy_index;
  }

  s32 sibling = find_best_sibling_(tree, box);

  s32 internal_node_index = node_alloc_(tree);
  Box_Node *internal_node = &tree->nodes[internal_node_index];
  {
    internal_node->box    = aabbv_union(tree->nodes[sibling].box, tree->nodes[leaf].box);
    internal_node->left   = sibling;
    internal_node->right  = leaf;
    internal_node->proxy  = -1;
    internal_node->parent = tree->nodes[sibling].parent;
  }

  if (tree->root == sibling)
  {
    tree->root = internal_node_index;
  }
  else
  {
    Box_Node *granny = &tree->nodes[tree->nodes[sibling].parent];
    if (granny->left == sibling)
    {
      granny->left = internal_node_index;
    }
    else
    {
      granny->right = internal_node_index;
    }
  }

  tree->nodes[sibling].parent = internal_node_index;
  tree->nodes[leaf].parent = internal_node_index;

  // Rotate and refit.
  s32 index = internal_node_index;
  while (index != -1)
  {
    rotate_(tree, index);
    Box_Node *node = &tree->nodes[index];
    tree->nodes[index].box = aabbv_union(tree->nodes[node->left].box, 
                                         tree->nodes[node->right].box);
    index = tree->nodes[index].parent;
  }

  return proxy_index;
}

void 
aabb_tree_remove(Box_Tree *tree, s32 proxy_index)
{
  Box_Proxy *proxy = &tree->proxies[proxy_index];
  s32 node_index = proxy->node;

  proxy_dealloc_(tree, proxy_index);

  Box_Node *node = &tree->nodes[node_index];
  if (node->parent == -1)
  {
    tree->root = -1;
    node_dealloc_(tree, node_index);
  }
  else
  {
    s32 sibling_index = tree->nodes[node->parent].left == node_index ? tree->nodes[node->parent].right : tree->nodes[node->parent].left;
    Box_Node *sibling = &tree->nodes[sibling_index];

    s32 parent_index = node->parent;
    Box_Node *parent = &tree->nodes[parent_index];

    s32 granny_index = parent->parent;

    sibling->parent = granny_index;
    if (granny_index != -1)
    {
      Box_Node *granny = &tree->nodes[granny_index];
      if (granny->left == parent_index)
      {
        granny->left = sibling_index;
      }
      else
      {
        granny->right = sibling_index;
      }
    }
    else
    {
      tree->root = sibling_index;
    }

    refit_from_(tree, sibling);

    node_dealloc_(tree, node_index);
    node_dealloc_(tree, parent_index);
  }
}

void
aabb_tree_move(Box_Tree *tree, s32 proxy_index, AABBV box)
{
  // @Important: Proxy index must be stable. Hence, the proxy free list 
  //             is last-in-first-out (aka, stack), so the insertion right 
  //             after the removal won't change the proxy index.
  aabb_tree_remove(tree, proxy_index);
  s32 new_proxy = aabb_tree_insert(tree, tree->proxies[proxy_index].item, box);
  R_ASSERT(new_proxy == proxy_index);
}

Array<Handle>
aabb_tree_query(Box_Tree *tree, 
                Ray      *ray, 
                Allocator allocator)
{
  Array<Handle> result(allocator);

  if (tree->root == -1)
  {
    return result;
  }

  s32 stack[AABB_TREE_STACK_SIZE];
  s32 top = 0;
  stack[0] = tree->root;

  while (top >= 0)
  {
    s32 index = stack[top--];
    Box_Node *node = &tree->nodes[index];
    R_ASSERT(top + 2 < AABB_TREE_STACK_SIZE);

    AABB box = aabb_from_aabbv(node->box);
    f32 t = INFINITY;
    if (ray_aabb_intersect(ray, &t, &box))
    {
      if (is_leaf_(node))
      {
        append(&result, tree->proxies[node->proxy].item);
      }
      else
      {
        if (node->left  != -1) stack[++top] = node->left;
        if (node->right != -1) stack[++top] = node->right;
      }
    }
  }

  return result;
}

Array<Handle>
aabb_tree_query(Box_Tree *tree, 
                m4x4     *view_proj,
                Allocator allocator)
{
  Array<Handle> result(allocator);

  if (tree->root == -1)
  {
    return result;
  }

  s32 stack[AABB_TREE_STACK_SIZE];
  s32 top = 0;
  stack[0] = tree->root;

  while (top >= 0)
  {
    s32 index = stack[top--];
    Box_Node *node = &tree->nodes[index];
    R_ASSERT(top + 2 < AABB_TREE_STACK_SIZE);

    AABB box = aabb_from_aabbv(node->box);
    if (view_frustum_aabb_intersect(view_proj, &box))
    {
      if (is_leaf_(node))
      {
        append(&result, tree->proxies[node->proxy].item);
      }
      else
      {
        if (node->left  != -1) stack[++top] = node->left;
        if (node->right != -1) stack[++top] = node->right;
      }
    }
  }

  return result;
}

void 
aabb_tree_visualize(Box_Tree *tree)
{
  if (tree->root == -1)
  {
    return;
  }

  s32 stack[AABB_TREE_STACK_SIZE];
  s32 top = 0;
  stack[0] = tree->root;

  while (top >= 0)
  {
    s32 index = stack[top--];
    Box_Node *node = &tree->nodes[index];
    R_ASSERT(top + 2 < AABB_TREE_STACK_SIZE);
    if (node->left  != -1) stack[++top] = node->left;
    if (node->right != -1) stack[++top] = node->right;

    vec4 color = vec4(1.0f, 0.3f, 0.0f, 1.0f);
    f32 thickness = 1.5f;
    if (is_leaf_(node))
    {
      color = vec4(0.2f, 0.2f, 1.0f, 1.0f);
    }
    draw_box(aabb_from_aabbv(node->box), game_state->camera.view_proj, color, thickness);
  }
}
