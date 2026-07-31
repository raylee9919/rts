// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"
#include "rhi/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "rhi/include.cpp"

OS_Handle window;
bool should_close    = false;
u32 surface_width    = 1920;
u32 surface_height   = 1080;
u32 num_back_buffers = 3;

int main_entry(int argc, char **argv)
{
    window = os_window_create(1920, 1080, utf8lit("rhi"));
    HWND hwnd = hwnd_from_os_handle(window);

    auto *rhi_device = (RHI_Device *)alloc(sizeof(RHI_Device));
    Assert(rhi_device_init(rhi_device, RHI_KIND_D3D12, true, true));

    RHI_Surface_Desc surface_desc = {};
    {
        surface_desc.native_window_handle = (void *)hwnd;
        surface_desc.width = surface_width;
        surface_desc.height = surface_height;
        surface_desc.num_back_buffers = num_back_buffers;
    }
    auto *rhi_surface = (RHI_Surface *)alloc(sizeof(RHI_Surface));
    Assert(rhi_surface_init(rhi_device, rhi_surface, surface_desc));


    auto *rhi_cmd_buffer = (RHI_Command_Buffer *)alloc(sizeof(RHI_Command_Buffer));
    Assert(rhi_command_buffer_init(rhi_device, rhi_cmd_buffer, RHI_COMMAND_TYPE_GRAPHICS));


    while (!should_close) {
        // Clear and poll events.
        os_clear_events();
        os_poll_events();

        // Close app if needed
        list_for(os->first_event, event) {
            // Alt + F4?
            bool alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            bool window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == window;


            if (alt_f4_pressed | window_close_triggered) {
                should_close = true;
                os_remove_event(event);
            }

            // Fullscreen
            bool alt_enter_pressed = event->kind == OS_EVENT_PRESS && event->key == KEY_RETURN && (event->modifiers & OS_MODIFIER_ALT);
            if (alt_enter_pressed) {
                os_window_toggle_fullscreen(window);
            }
        }

        // Render
        {
            rhi_surface_present(rhi_surface);
        }

        clear_temporary_storage();
    }

    rhi_command_buffer_deinit(rhi_cmd_buffer);
    rhi_device_deinit(rhi_device);

    return 0;
}
