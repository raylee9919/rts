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
bool should_close = false;

int main_entry(int argc, char **argv)
{
    window = os_create_window(1920, 1080, utf8lit("rhi"));

    auto *rhi_device = (RHI_Device *)alloc(sizeof(RHI_Device));
    Assert(rhi_device_init(rhi_device, RHI_KIND_D3D12, true, true));

    auto *rhi_list = (RHI_Command_List *)alloc(sizeof(RHI_Command_List));
    Assert(rhi_command_list_init(rhi_device, rhi_list, RHI_COMMAND_TYPE_GRAPHICS));

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
        }

        clear_temporary_storage();
    }

    rhi_command_list_deinit(rhi_list);
    rhi_device_deinit(rhi_device);

    return 0;
}
