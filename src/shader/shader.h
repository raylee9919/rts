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
};

struct Shader_Compile_Result {
    Shader_Stage stage;

    u8 *data;
    u64 size;

    u32 num_outputs;
    u32 num_instructions;
    u32 num_texture_loads; // Texture load instructions
};

internal bool shader_compiler_init(Shader_Compiler *compiler);
internal void shader_compiler_deinit(Shader_Compiler *compiler);
// Allocator allocates memory for the compiled binary blob.
internal bool shader_compile(Shader_Compiler *compiler, Shader_Compile_Options options, bool debug, Shader_Compile_Result *out_result, Allocator allocator);
