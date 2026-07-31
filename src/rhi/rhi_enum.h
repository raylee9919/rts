// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RHI_ENUM_H
#define RHI_ENUM_H


#define RHI_MAX_BACK_BUFFER         3
#define RHI_MAX_COLOR_ATTACHMENTS   8 // @Study


typedef u8 RHI_Command_Type;
enum {
    RHI_COMMAND_TYPE_GRAPHICS,
    RHI_COMMAND_TYPE_COMPUTE,
    RHI_COMMAND_TYPE_TRANSFER,

    RHI_COMMAND_TYPE_COUNT
};

struct RHI_Surface_Desc {
    void *native_window_handle; // This isn't a pointer to the handle. It's a handle itself.
    u32   width;
    u32   height;
    u32   num_back_buffers;
};


#endif // RHI_ENUM_H
