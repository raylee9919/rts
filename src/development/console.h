// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_CONSOLE_H
#define RTS_CONSOLE_H

#include "basic/core.h"
#include "math/math.h"

struct Console {
    b32 open   = false; // if not set, closed.
    f32 t      = 0.f;
    f32 open_t = 0.25f;
    vec4 color = vec4(0.12f, 0.08f, 0.08f, 0.95f);
};

extern Console console;

void ToggleConsole();
void UpdateConsole(f32 dt);

#endif // RTS_CONSOLE_H
