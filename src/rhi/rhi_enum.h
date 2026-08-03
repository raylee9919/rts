// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RHI_ENUM_H
#define RHI_ENUM_H


#define RHI_MAX_BACK_BUFFERS        3
#define RHI_MAX_COLOR_ATTACHMENTS   8 // @Study
#define RHI_SURFACE_FORMAT          RHI_TEXTURE_FORMAT_RGBA8_UNORM


struct RHI_Device;
struct RHI_Command_Buffer;
struct RHI_Pass;
struct RHI_Texture;
struct RHI_Texture_Desc;
struct RHI_Texture_View;
struct RHI_Texture_View_Desc;
struct RHI_Heap;
struct RHI_Surface;
struct RHI_Surface_Desc;
struct RHI_Fence;


typedef u8 RHI_Command_Type;
enum {
    RHI_COMMAND_TYPE_GRAPHICS,
    RHI_COMMAND_TYPE_COMPUTE,
    RHI_COMMAND_TYPE_TRANSFER,

    RHI_COMMAND_TYPE_COUNT
};

enum RHI_Resource_State {
    RHI_RESOURCE_STATE_COMMON,
    RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    RHI_RESOURCE_STATE_INDEX_BUFFER,
    RHI_RESOURCE_STATE_RENDER_TARGET,
    RHI_RESOURCE_STATE_UNORDERED_ACCESS,
};


#endif // RHI_ENUM_H
