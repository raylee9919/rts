// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_TYPE_H
#define RTS_TYPE_H

#include "basic/string.h"

struct Field {
};

struct Type_Info {
  String  name;
  s64     size;
  s64     alignment;

  s64     num_fields;
  Field   *fields;
};

#endif // RTS_TYPE_H
