// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef TEST_H
#define TEST_H

#include "meta/meta.h"

struct Bar {
  float health;
  int   mana;
};

struct REFLECT() Foo 
{
  FIELD() 
  float x;

  Bar bar;

  FIELD() 
  char *c;
};

#endif // TEST_H
