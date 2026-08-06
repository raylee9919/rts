// Copyright Seong Woo Lee. All Rights Reserved.

static String shader_profile_from_stage(Shader_Stage stage) {
    switch (stage) {
        case SHADER_STAGE_VS: return S("vs_6_6");
        case SHADER_STAGE_PS: return S("ps_6_6");
        case SHADER_STAGE_CS: return S("cs_6_6");
        case SHADER_STAGE_MS: return S("ms_6_6");
        case SHADER_STAGE_TS: return S("as_6_6");
        default:
            Assert(!"Undefined shader stage.");
            return S("null");
    }
}

bool shader_compiler_init(Shader_Compiler *compiler) {
    bool success = false;

    // Starting from DXC, 'Utils' replaces 'Library'.
    IDxcCompiler3* compiler_3           = NULL;
    IDxcUtils* utils                    = NULL;
    IDxcIncludeHandler *include_handler = NULL;

    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler_3)))) {
        return false;
    }

    if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))) {
        COM_SAFE_RELEASE(&compiler_3);
        return false;
    }

    if (FAILED(utils->CreateDefaultIncludeHandler(&include_handler))) {
        COM_SAFE_RELEASE(&compiler_3);
        COM_SAFE_RELEASE(&utils);
        return false;
    }

    compiler->compiler_3      = compiler_3;
    compiler->utils           = utils;
    compiler->include_handler = include_handler;

    log(LOG_INFO, S("Initialized DXC."));
    return true;
}

void shader_compiler_deinit(Shader_Compiler *compiler) {
    COM_SAFE_RELEASE(&compiler->compiler_3);
    COM_SAFE_RELEASE(&compiler->utils);

    log(LOG_INFO, S("Deinitialized DXC."));
}

// @Todo: Proper cleanup when failed.
bool shader_compile(Shader_Compiler *compiler, Shader_Compile_Options options, bool debug, 
                    Shader_Compile_Result *out_result, Allocator allocator) {
    HRESULT hr = S_OK;

    IDxcResult *compile_result         = NULL;
    IDxcBlobUtf8 *error_msgs           = NULL;
    IDxcBlob *shader_blob              = NULL;
    IDxcBlob *reflection_blob          = NULL;
    ID3D12ShaderReflection *reflection = NULL;
    IDxcResult *disasm_result          = NULL;
    IDxcBlobUtf8 *disasm_text          = NULL;


    // @Todo: Not fan of this. Should I just init an arena in the compiler?
    Utf16 profile16 = to_utf16(tctx.temp, shader_profile_from_stage(options.stage));
    Utf16 entry16   = to_utf16(tctx.temp, options.entry);


    // Make source buffer
    DxcBuffer source_buffer = {};
    {
        source_buffer.Ptr      = options.source.str;
        source_buffer.Size     = options.source.len;
        source_buffer.Encoding = DXC_CP_UTF8;
    };


    // Pack arguments. Use "-" instead of "/".
    // https://simoncoenen.com/blog/programming/graphics/DxcCompiling
    LPCWCHAR args[32];
    UINT32 num_args = 0;
    {
        // Entry point name (e.g. main_vs)
        args[num_args++] = L"-E";                       
        args[num_args++] = (LPCWCHAR)entry16.str;

        // Target profile (e.g. ps_6_6)
        args[num_args++] = L"-T";                       
        args[num_args++] = (LPCWCHAR)profile16.str;

        args[num_args++] = L"-WX";                      // Treat warnings as errors
        args[num_args++] = L"-Zpr";                     // Pack matrices in row-major order


        if (debug) {
            args[num_args++] = L"-Zi";                  // Enable debug information
            args[num_args++] = L"-Qembed_debug";        // Embed debug symbols
            args[num_args++] = L"-Od";                  // Disable optimization
        } else {
            args[num_args++] = L"-O3";                  // Optimization Level 3 (Default)
            args[num_args++] = L"-all_resources_bound"; // Driver-side optimization
        }
    }
    Assert(num_args <= array_count(args));


    // Compile source blob
    hr = compiler->compiler_3->Compile(&source_buffer, args, num_args, 
                                       compiler->include_handler, IID_PPV_ARGS(&compile_result));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("IDxcCompiler3::Compile failed."));
        return false;
    }


    // Log messages
    hr = compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), NULL);
    if (SUCCEEDED(hr)) {
        if (error_msgs && error_msgs->GetStringLength() > 0) {
            log(LOG_ERROR, S("Error message from DXC: %s"), (char *)error_msgs->GetBufferPointer());
        }
    }


    // Status
    HRESULT status;
    if (FAILED(compile_result->GetStatus(&status)) || FAILED(status)) {
        log(LOG_ERROR, S("Encountered bad status during DXC shader compilation."));
        return false;
    }


    // Result object
    hr = compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), NULL);
    if (FAILED(hr)) {
        log(LOG_ERROR, S("GetOutput(DXC_OUT_OBJECT, ..) failed."));
        return false;
    }


    // Copy blob to the output result
    u64 size = shader_blob->GetBufferSize();
    u8 *blob = (u8 *)shader_blob->GetBufferPointer();
    out_result->size = size;
    out_result->data = (u8 *)alloc(size, allocator);
    memcpy(out_result->data, blob, size);
    out_result->stage = options.stage;


    // Reflection
    D3D12_SHADER_DESC shader_desc = {};
    {
        hr = compile_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection_blob), NULL);
        if (FAILED(hr)) {
            log(LOG_ERROR, S("GetOutput(DXC_OUT_REFLECTION, ..) failed."));
            return false;
        }

        DxcBuffer reflection_buffer = {};
        reflection_buffer.Ptr      = reflection_blob->GetBufferPointer();
        reflection_buffer.Size     = reflection_blob->GetBufferSize();
        reflection_buffer.Encoding = 0;

        compiler->utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&reflection));
        reflection->GetDesc(&shader_desc);

        out_result->num_outputs       = shader_desc.OutputParameters;
        out_result->num_instructions  = shader_desc.InstructionCount;
        out_result->num_texture_loads = shader_desc.TextureLoadInstructions;

    }


    // Disassembly
    {
        DxcBuffer buf = {};
        buf.Ptr      = shader_blob->GetBufferPointer();
        buf.Size     = shader_blob->GetBufferSize();
        buf.Encoding = 0;

        hr = compiler->compiler_3->Disassemble(&buf, IID_PPV_ARGS(&disasm_result));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("IDxcCompiler3::Disassemble failed."));
        } else {
            hr = disasm_result->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(&disasm_text), NULL);
            if (SUCCEEDED(hr) && disasm_text && disasm_text->GetStringLength() > 0) {
                // do something.
            }
        }
    }

    // Cleanup
    COM_SAFE_RELEASE(&compile_result);
    COM_SAFE_RELEASE(&error_msgs);
    COM_SAFE_RELEASE(&shader_blob);
    COM_SAFE_RELEASE(&reflection_blob);
    COM_SAFE_RELEASE(&reflection);
    COM_SAFE_RELEASE(&disasm_result);
    COM_SAFE_RELEASE(&disasm_text);


    log(LOG_INFO, S("Compiled shader: outputs: %u, instructions: %u, texture loads: %u"), 
        out_result->num_outputs,
        out_result->num_instructions,
        out_result->num_texture_loads);

    return true;
}
