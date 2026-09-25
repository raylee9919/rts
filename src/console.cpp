// Copyright Seong Woo Lee. All Rights Reserved.

#include "./console.h"
#include "basic/log.h"
#include "renderer/renderer.h"
#include "renderer/immediate.h"

Console console;

static void OpenConsole()
{
    console.open = true;
}

static void CloseConsole()
{
    console.open = false;
}

void ToggleConsole()
{
    if (console.open) {
        log_info(S("Close console."));
        CloseConsole();
    } else {
        log_info(S("Open console."));
        OpenConsole();
    }
}

void UpdateConsole(f32 dt)
{
    if (console.open) {
        console.t = clamp(console.t + dt, 0.0f, console.open_t);
    } else {
        console.t = clamp(console.t - dt, 0.0f, console.open_t);
    }

    vec4 color = console.color;
    f32 t      = 1.0f - (console.open_t - console.t) / console.open_t; // [0,1]
    f32 height = RESOLUTION_Y * 0.25f;

    draw_quad(vec2(0.0f), 
              vec2(0.0f, t * height), 
              vec2(RESOLUTION_X, 0.0f), 
              vec2(RESOLUTION_X, t * height), 
              color, color, color, color);
}
