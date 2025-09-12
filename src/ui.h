/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


struct Ui_Element 
{
    u32 id;
    v2 position;
    Ui_Element *next;
};

struct Ui_Hashmap 
{
    Ui_Element entries[4096];
};

struct Ui_Fadeout_Text 
{
    f32 t;
    char *text;
    v4 color;
};

struct Ui 
{
    b32 initted;

    struct Input *input;
    struct Mouse_Input *mouse;
    struct Arena *arena;
    struct Render_Group *render_group;
    struct Asset_Font *font;
    struct Asset_Font *bigfont;

    u32 hot;
    u32 active;

    v2 top_left;

    u32 crc32_lut[256];

    Ui_Hashmap hashmap;

    Ui_Fadeout_Text fadeout_texts[32];
    u32 next_fadeout_text;

    void init(Input *input, Arena *arena, Render_Group *render_group, Asset_Font *font, Asset_Font *bigfont);
    b32 hot_begins(Rect2 rect);
    void begin(char *label, v2 top_left = v2{}, b32 attachanchor = true);
    void end();
    void next(f32 itemheight);
    void description(char *description);
    void text(v4 color, char *text);
    void fadeout_text(v4 color, char *text);
    b32 button(v4 color, char *text, char *description = 0);
    b32 checkbox(b32 *data, v4 default_color, char *text, char *description = 0);
    b32 slider(f32 *data, f32 min, f32 max, v4 default_color, char *text, char *desc = 0);
    void gizmo(v3 *position, m4x4 view_proj);
    void end_frame();
};
