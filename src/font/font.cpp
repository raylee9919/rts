// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/core.h"

#if OS_WINDOWS
#  include "./dwrite/direct_write.cpp"
#else
#  error UNDEFINED OS
#endif
