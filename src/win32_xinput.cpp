/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

#define XINPUT_GET_STATE(Name) DWORD Name(DWORD, XINPUT_STATE *)
typedef XINPUT_GET_STATE(XInput_Get_State);
XINPUT_GET_STATE(xinput_get_state_stub) { return 1; }
XInput_Get_State *xinput_get_state = xinput_get_state_stub;

#define XINPUT_SET_STATE(name) DWORD name(DWORD, XINPUT_VIBRATION *)
typedef XINPUT_SET_STATE(XInput_Set_State);
XINPUT_SET_STATE(xinput_set_state_stub) { return 1; }
XInput_Set_State *xinput_set_state = xinput_set_state_stub;

internal void
win32_load_xinput() 
{
    HMODULE xinput_dll = LoadLibraryW(L"xinput.dll");
    if (!xinput_dll) {
        // TODO: diagnostic.
    }

    HMODULE xinput_module = LoadLibraryA("xinput1_4.dll");
    if (!xinput_module) 
        LoadLibraryA("Xinput9_1_0.dll"); 
    if (!xinput_module) 
        LoadLibraryA("xinput1_3.dll"); 

    if(xinput_module) {
        xinput_get_state = (XInput_Get_State *)GetProcAddress(xinput_module, "XInputGetState");
        xinput_set_state = (XInput_Set_State *)GetProcAddress(xinput_module, "XInputSetState");
    } else {
        xinput_get_state = xinput_get_state_stub;
        xinput_set_state = xinput_set_state_stub;
    }
}
