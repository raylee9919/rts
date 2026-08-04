// Copyright Seong Woo Lee. All Rights Reserved.

struct Shader_Compiler;

enum Shader_Stage : u8 {
    SHADER_STAGE_NULL = 0,
    SHADER_STAGE_VS,        // Vertex shader
    SHADER_STAGE_PS,        // Pixel (fragment) shader
    SHADER_STAGE_CS,        // Compute shader
    SHADER_STAGE_MS,        // Mesh shader
    SHADER_STAGE_TS,        // Task (amplification) shader
};

struct Shader_Compile_Options {
    Shader_Stage stage;
    String       entry;
    String       source;
    b32          debug;
};

struct Shader_Compile_Result {
    Shader_Stage stage;
    u8 *data;
    u64 size;
};

internal bool shader_compiler_init(Shader_Compiler *compiler);
internal void shader_compiler_deinit(Shader_Compiler *compiler);
internal bool shader_compile(Shader_Compiler *compiler, Shader_Compile_Options options, Shader_Compile_Result *out_result, Allocator allocator);
