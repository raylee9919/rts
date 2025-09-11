/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

internal void
win32_map_keycode_to_hid_key_code(u8 *map) 
{
    map[VK_BACK]      = KEY_BACKSPACE;
    map[VK_TAB]       = KEY_TAB;
    map[VK_RETURN]    = KEY_ENTER;
    map[VK_SHIFT]     = KEY_LEFTSHIFT;
    map[VK_LSHIFT]    = KEY_LEFTSHIFT;
    map[VK_CONTROL]   = KEY_LEFTCTRL;
    map[VK_LCONTROL]  = KEY_LEFTCTRL;
    map[VK_LMENU]     = KEY_LEFTALT;
    map[VK_ESCAPE]    = KEY_ESC;
    map[VK_SPACE]     = KEY_SPACE;
    map[VK_OEM_3]     = KEY_HASHTILDE;
    map[VK_LEFT]      = KEY_LEFT;
    map[VK_RIGHT]     = KEY_RIGHT;
    map[VK_UP]        = KEY_UP;
    map[VK_DOWN]      = KEY_DOWN;
    map[VK_OEM_PLUS]  = KEY_EQUAL;
    map[VK_OEM_MINUS] = KEY_MINUS;
    for (char c = 'A'; c <= 'Z'; ++c)
        map[c] = KEY_A + (c - 'A');
    for (char c = 0x30; c <= 0x39; ++c) // 0~9
        map[c] = KEY_0 + (c - 0x30);
    for (char c = VK_F1; c <= VK_F12; ++c)
        map[c] = KEY_F1 + (c - VK_F1);
}

