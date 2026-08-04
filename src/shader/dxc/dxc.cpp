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
        SAFE_RELEASE(&compiler_3);
        return false;
    }

    if (FAILED(utils->CreateDefaultIncludeHandler(&include_handler))) {
        SAFE_RELEASE(&compiler_3);
        SAFE_RELEASE(&utils);
        return false;
    }

    compiler->compiler_3      = compiler_3;
    compiler->utils           = utils;
    compiler->include_handler = include_handler;

    log(LOG_INFO, S("Initialized DXC."));
    return true;
}

void shader_compiler_deinit(Shader_Compiler *compiler) {
    SAFE_RELEASE(&compiler->compiler_3);
    SAFE_RELEASE(&compiler->utils);

    log(LOG_INFO, S("Deinitialized DXC."));
}

// @Todo: Proper cleanup when failed.
bool shader_compile(Shader_Compiler *compiler, Shader_Compile_Options options, 
                    Shader_Compile_Result *out_result, Allocator allocator) {
    HRESULT hr = S_OK;

    IDxcResult *compile_result = NULL;
    IDxcBlobUtf8 *error_msgs   = NULL;
    IDxcBlob *shader_blob      = NULL;


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


        if (options.debug) {
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


    // Status
    HRESULT status;
    if (FAILED(compile_result->GetStatus(&status)) || FAILED(status)) {
        log(LOG_ERROR, S("Encountered bad status during DXC shader compilation."));

        // Log error messages
        hr = compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), NULL);
        if (SUCCEEDED(hr)) {
            if (error_msgs && error_msgs->GetStringLength() > 0) {
                log(LOG_ERROR, S("Error message from DXC: %s"), (char *)error_msgs->GetBufferPointer());
            }
        } else {
            log(LOG_ERROR, S("GetOutput(DXC_OUT_ERRORS, ..) failed."));
        }

        return false;
    }


    // Result object
    hr = compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), NULL);
    if (FAILED(hr)) {
        log(LOG_ERROR, S("GetOutput(DXC_OUT_OBJECT, ..) failed."));
        return false;
    }


    // Copy blob to output result
    u64 size = shader_blob->GetBufferSize();
    u8 *blob = (u8 *)shader_blob->GetBufferPointer();
    out_result->size = size;
    out_result->data = (u8 *)alloc(size, allocator);
    memcpy(out_result->data, blob, size);
    out_result->stage = options.stage;


    // Cleanup
    SAFE_RELEASE(&shader_blob);
    SAFE_RELEASE(&error_msgs);
    SAFE_RELEASE(&compile_result);


    log(LOG_INFO, S("Compiled shader."));
    return true;
}
