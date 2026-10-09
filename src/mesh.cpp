// Copyright Seong Woo Lee. All Rights Reserved.

#include "./mesh.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string_builder.h"

String
string_from_mesh(Triangle_Mesh *mesh, Allocator allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S(""));

  s = flush(&b);
  return s;
}
