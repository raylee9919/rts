// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_INCLUDE_H
#define RTS_OS_INCLUDE_H


// OS Include
//
#if OS_WINDOWS
#  include "os/win32/os_win32.h"
#else
#  error Undefined OS
#endif


#include "./os.h"


#endif // RTS_OS_INCLUDE_H
