// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef ASSET_H
#define ASSET_H

#include "basic/core.h"
#include "basic/string.h"

#define MAX_NAME_LENGTH 256

struct Name {
  u8     data[MAX_NAME_LENGTH + 1];
  s64    count;
};

FORCEINLINE Name
make_name(u8 *data, s64 count)
{
  count = min(count, (s64)MAX_NAME_LENGTH);
  Name name = {};
  name.count = count;
  memcpy(name.data, data, count);
  return name;
}

#endif // ASSET_H
