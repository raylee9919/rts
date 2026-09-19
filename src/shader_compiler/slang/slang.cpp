// Copyright Seong Woo Lee. All Rights Reserved.

#include "shader_compiler/slang/slang.h"
#include "shader_compiler/shader.h"
#include "basic/context.h"
#include "basic/log.h"
#include "os/os.h"

#include "third_party/slang/include/slang-com-ptr.h"

static SlangStage slang_stage_from_shader_stage(Shader_Stage stage) {
    switch (stage) {
        case SHADER_STAGE_VS: return SLANG_STAGE_VERTEX;
        case SHADER_STAGE_PS: return SLANG_STAGE_FRAGMENT;
        case SHADER_STAGE_CS: return SLANG_STAGE_COMPUTE;
        case SHADER_STAGE_MS: return SLANG_STAGE_MESH;
        case SHADER_STAGE_TS: return SLANG_STAGE_AMPLIFICATION;
        default:
            Assert(!"Undefined shader stage.");
            return SLANG_STAGE_NONE;
    }
}

static void log_diagnostics(slang::IBlob *diagnostics) {
    if (diagnostics && diagnostics->getBufferSize() > 0) {
        log(LOG_ERROR, S("Error message from Slang: %s"), (char *)diagnostics->getBufferPointer());
    }
}

bool shader_compiler_init(Shader_Compiler *compiler) {
    slang::IGlobalSession *global_session = NULL;

    if (SLANG_FAILED(slang::createGlobalSession(&global_session))) {
        log_error(S("Failed to initialize Slang."));
        return false;
    }

    compiler->global_session = global_session;

    log_info(S("Initialized Slang."));
    return true;
}

void shader_compiler_shutdown(Shader_Compiler *compiler) {
    COM_SAFE_RELEASE(&compiler->global_session);

    log_info(S("Deinitialized Slang."));
}

bool shader_compile(Shader_Compiler *compiler,
                    Shader_Compile_Options options,
                    bool debug,
                    Shader_Compile_Result *out_result,
                    Allocator allocator) {
    Slang::ComPtr<slang::ISession>        session;
    Slang::ComPtr<slang::IBlob>           diagnostics;
    Slang::ComPtr<slang::IEntryPoint>     entry_point;
    Slang::ComPtr<slang::IComponentType>  composite;
    Slang::ComPtr<slang::IComponentType>  linked;
    Slang::ComPtr<slang::IBlob>           shader_blob;


    // Slang wants null-terminated strings.
    char *source = (char *)str_copy(options.source, tctx.temp).str;
    char *path   = (char *)str_copy(options.path.str ? options.path : S("shader.slang"), tctx.temp).str;

    char *material_source = options.material_source.str ? (char *)str_copy(options.material_source, tctx.temp).str : NULL;
    char *material_path   = (char *)str_copy(options.material_path.str ? options.material_path : S("material.slang"), tctx.temp).str;


    // Target
    slang::TargetDesc target = {};
    {
        target.format  = SLANG_DXIL;
        target.profile = compiler->global_session->findProfile("sm_6_6");
    }


    // Compiler options
    slang::CompilerOptionEntry option_entries[8];
    u32 num_option_entries = 0;
    {
        slang::CompilerOptionEntry *e;

        // Treat warnings as errors
        e = &option_entries[num_option_entries++];
        e->name               = slang::CompilerOptionName::WarningsAsErrors;
        e->value.kind         = slang::CompilerOptionValueKind::String;
        e->value.stringValue0 = "all";

        if (debug) {
            e = &option_entries[num_option_entries++];
            e->name            = slang::CompilerOptionName::DebugInformation;
            e->value.kind      = slang::CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;

            e = &option_entries[num_option_entries++];
            e->name            = slang::CompilerOptionName::Optimization;
            e->value.kind      = slang::CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE;
        } else {
            e = &option_entries[num_option_entries++];
            e->name            = slang::CompilerOptionName::Optimization;
            e->value.kind      = slang::CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_OPTIMIZATION_LEVEL_HIGH;
        }
    }
    Assert(num_option_entries <= array_count(option_entries));


    // Session
    {
        const char *search_paths[1];
        u32 num_search_paths = 0;
        if (compiler->include_path.str) {
            search_paths[num_search_paths++] = (char *)str_copy(compiler->include_path, tctx.temp).str;
        }

        slang::SessionDesc desc = {};
        desc.targets                  = &target;
        desc.targetCount              = 1;
        desc.defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_ROW_MAJOR;  // Pack matrices in row-major order
        desc.searchPaths              = search_paths;
        desc.searchPathCount          = num_search_paths;
        desc.compilerOptionEntries    = option_entries;
        desc.compilerOptionEntryCount = num_option_entries;

        if (SLANG_FAILED(compiler->global_session->createSession(desc, session.writeRef()))) {
            log_error(S("IGlobalSession::createSession failed."));
            return false;
        }
    }


    // Load module
    slang::IModule *module = session->loadModuleFromSourceString(path, path, source, diagnostics.writeRef());
    log_diagnostics(diagnostics);
    if (!module) {
        log_error(S("ISession::loadModuleFromSourceString failed."));
        return false;
    }


    // Material module
    slang::IModule *material_module = NULL;
    if (material_source) {
        material_module = session->loadModuleFromSourceString(material_path, material_path, material_source, diagnostics.writeRef());
        log_diagnostics(diagnostics);
        if (!material_module) {
            log_error(S("%s: Failed to load material."), material_path);
            return false;
        }
    }


    // Entry point: whichever [shader("...")] function matches the requested stage.
    SlangStage stage = slang_stage_from_shader_stage(options.stage);
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i) {
        Slang::ComPtr<slang::IEntryPoint> candidate;
        if (SLANG_FAILED(module->getDefinedEntryPoint(i, candidate.writeRef()))) {
            continue;
        }

        if (candidate->getLayout()->getEntryPointByIndex(0)->getStage() != stage) {
            continue;
        }

        if (entry_point) {
            log_error(S("%s: Multiple entry points for the same shader stage."), path);
            return false;
        }
        entry_point = candidate;
    }

    if (!entry_point) {
        log_error(S("%s: No entry point for the requested shader stage."), path);
        return false;
    }

    const char *entry = entry_point->getLayout()->getEntryPointByIndex(0)->getName();


    // Compose and link
    {
        slang::IComponentType *components[3];
        SlangInt num_components = 0;
        components[num_components++] = module;
        if (material_module) {
            components[num_components++] = material_module;
        }
        components[num_components++] = entry_point;

        if (SLANG_FAILED(session->createCompositeComponentType(components, num_components,
                                                               composite.writeRef(), diagnostics.writeRef()))) {
            log_diagnostics(diagnostics);
            log_error(S("ISession::createCompositeComponentType failed."));
            return false;
        }

        if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef()))) {
            log_diagnostics(diagnostics);
            log_error(S("IComponentType::link failed."));
            return false;
        }
    }


    // Result object
    if (SLANG_FAILED(linked->getEntryPointCode(0, 0, shader_blob.writeRef(), diagnostics.writeRef()))) {
        log_diagnostics(diagnostics);
        log_error(S("IComponentType::getEntryPointCode failed."));
        return false;
    }


    // Copy blob to the output result
    u64 size = shader_blob->getBufferSize();
    u8 *blob = (u8 *)shader_blob->getBufferPointer();
    out_result->size = size;
    out_result->data = (u8 *)alloc(size, allocator);
    memcpy(out_result->data, blob, size);
    out_result->stage = options.stage;


    // Reflection
    {
        slang::EntryPointReflection *entry_reflection = linked->getLayout()->getEntryPointByIndex(0);
        slang::VariableLayoutReflection *result_layout = entry_reflection->getResultVarLayout();
        slang::TypeLayoutReflection *result_type = result_layout ? result_layout->getTypeLayout() : NULL;

        if (!result_type || result_type->getKind() == slang::TypeReflection::Kind::None) {
            out_result->num_outputs = 0;
        } else if (result_type->getKind() == slang::TypeReflection::Kind::Struct) {
            out_result->num_outputs = result_type->getFieldCount();
        } else if (result_type->getKind() == slang::TypeReflection::Kind::Scalar &&
                   result_type->getScalarType() == slang::TypeReflection::ScalarType::Void) {
            out_result->num_outputs = 0;
        } else {
            out_result->num_outputs = 1;
        }
    }


    log_info(S("Slang compiled shader: %s, outputs: %u"), entry, out_result->num_outputs);

    return true;
}
