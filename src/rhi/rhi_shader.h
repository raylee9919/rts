// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RHI_SHADER_H
#define RHI_SHADER_H


enum RHI_Shader_Stage {
    RHI_SHADER_STAGE_VERTEX,
    RHI_SHADER_STAGE_PIXEL,
    RHI_SHADER_STAGE_COMPUTE,
};

internal void rhi_shader_compile(String source);



#endif // RHI_SHADER_H
