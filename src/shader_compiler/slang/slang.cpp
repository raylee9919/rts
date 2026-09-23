// Copyright Seong Woo Lee. All Rights Reserved.

#include "shader_compiler/slang/slang.h"
#include "shader_compiler/shader.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string_builder.h"
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
            R_ASSERT(!"Undefined shader stage.");
            return SLANG_STAGE_NONE;
    }
}

static void log_diagnostics(slang::IBlob *diagnostics) {
    if (diagnostics && diagnostics->getBufferSize() > 0) {
        log(LOG_ERROR, S("Error message from Slang: %s"), (char *)diagnostics->getBufferPointer());
    }
}

static Shader_Field_Type
get_shader_field_type(slang::TypeLayoutReflection *l, const char *file) 
{
    using namespace slang;

    if ( l->getKind() == TypeReflection::Kind::Scalar ) {
        switch(l->getScalarType())
        {
            case TypeReflection::Float32:   return SHADER_FIELD_FLOAT;

            case TypeReflection::UInt8:     return SHADER_FIELD_UINT8;
            case TypeReflection::UInt16:    return SHADER_FIELD_UINT16;
            case TypeReflection::UInt32:    return SHADER_FIELD_UINT32;
            case TypeReflection::UInt64:    return SHADER_FIELD_UINT64;

            case TypeReflection::Int8:      return SHADER_FIELD_INT8;
            case TypeReflection::Int16:     return SHADER_FIELD_INT16;
            case TypeReflection::Int32:     return SHADER_FIELD_INT32;
            case TypeReflection::Int64:     return SHADER_FIELD_INT64;

            default: {
                log_error( S("Error in '%s': Unhandled field type encountered in '%s'."), file );
                return SHADER_FIELD_INVALID;
            }
        }
    } else if (l->getKind() == TypeReflection::Kind::Vector) {
        s64 n = l->getElementCount();
        if (n == 1) {
            log_error( S("Error in '%s': number of elements in a vector cannot be 1. It might be a bug in Slang."), file );
            return SHADER_FIELD_INVALID;
        }

        if (n > 4) {
            log_error( S("Error in '%s': number of elements in a vector cannot exceed 4."), file );
            return SHADER_FIELD_INVALID;
        }

        auto t = l->getScalarType();
        if (t == TypeReflection::ScalarType::Float32) {
            if (n == 2) {
                return SHADER_FIELD_FLOAT2;
            } else if (n == 3) {
                return SHADER_FIELD_FLOAT3;
            } else {
                return SHADER_FIELD_FLOAT4;
            }
        } else {
            log_error(S("Encountered unhandled scalar type in '%s'"), file);
            return SHADER_FIELD_INVALID;
        }
    } else {
        log_error( S("Encountered unhandled field kind in '%s'"), file );
        return SHADER_FIELD_INVALID;
    }
}

String string_from_shader_field_type(Shader_Field_Type type)
{
    switch (type) {
        case SHADER_FIELD_FLOAT : return String(S("float"));
        case SHADER_FIELD_FLOAT2: return String(S("float2"));
        case SHADER_FIELD_FLOAT3: return String(S("float3"));
        case SHADER_FIELD_FLOAT4: return String(S("float4"));

        case SHADER_FIELD_UINT8  : return String(S("uint8_t"));
        case SHADER_FIELD_UINT16 : return String(S("uint16_t"));
        case SHADER_FIELD_UINT32 : return String(S("uint32_t"));
        case SHADER_FIELD_UINT64 : return String(S("uint64_t"));

        case SHADER_FIELD_INT8  : return String(S("int8_t"));
        case SHADER_FIELD_INT16 : return String(S("int16_t"));
        case SHADER_FIELD_INT32 : return String(S("int32_t"));
        case SHADER_FIELD_INT64 : return String(S("int64_t"));

        default: {
            R_ASSERT(!"Unhandled shader field type.");
            return {};
        } break;
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

static bool shader_session_create(Shader_Compiler *compiler, bool debug, slang::ISession **out_session)
{
    using namespace slang;

    // Target
    TargetDesc target = {};
    target.format  = SLANG_DXIL;
    target.profile = compiler->global_session->findProfile("sm_6_6");


    // Compiler options
    CompilerOptionEntry option_entries[8];
    u32 num_option_entries = 0;
    {
        CompilerOptionEntry *e;

        // Treat warnings as errors
        e = &option_entries[num_option_entries++];
        e->name               = CompilerOptionName::WarningsAsErrors;
        e->value.kind         = CompilerOptionValueKind::String;
        e->value.stringValue0 = "all";

        if (debug) {
            e = &option_entries[num_option_entries++];
            e->name            = CompilerOptionName::DebugInformation;
            e->value.kind      = CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;

            e = &option_entries[num_option_entries++];
            e->name            = CompilerOptionName::Optimization;
            e->value.kind      = CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE;
        } else {
            e = &option_entries[num_option_entries++];
            e->name            = CompilerOptionName::Optimization;
            e->value.kind      = CompilerOptionValueKind::Int;
            e->value.intValue0 = SLANG_OPTIMIZATION_LEVEL_HIGH;
        }
    }
    R_ASSERT(num_option_entries <= array_count(option_entries));


    // Session
    {
        const char *search_paths[1];
        u32 num_search_paths = 0;
        if (compiler->include_path.str) {
            search_paths[num_search_paths++] = (char *)copy_string(compiler->include_path, tctx.temp).str;
        }

        SessionDesc desc = {};
        desc.targets                  = &target;
        desc.targetCount              = 1;
        desc.defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_ROW_MAJOR;  // Pack matrices in row-major order
        desc.searchPaths              = search_paths;
        desc.searchPathCount          = num_search_paths;
        desc.compilerOptionEntries    = option_entries;
        desc.compilerOptionEntryCount = num_option_entries;

        if (SLANG_FAILED(compiler->global_session->createSession(desc, out_session))) {
            log_error(S("IGlobalSession::createSession failed."));
            return false;
        }
    }

    return true;
}

bool shader_compile(Shader_Compiler *compiler,
                    Shader_Compile_Options options,
                    bool debug,
                    Shader_Compile_Result *out_result,
                    Allocator allocator) 
{
    using namespace slang;

    Slang::ComPtr<slang::ISession>        session;
    Slang::ComPtr<slang::IBlob>           diagnostics;
    Slang::ComPtr<slang::IEntryPoint>     entry_point;
    Slang::ComPtr<slang::IComponentType>  composite;
    Slang::ComPtr<slang::IComponentType>  linked;
    Slang::ComPtr<slang::IBlob>           shader_blob;


    // Slang wants null-terminated strings.
    char *source = (char *)copy_string(options.source, tctx.temp).str;
    char *path   = (char *)copy_string(options.path.str ? options.path : S("shader.slang"), tctx.temp).str;

    char *material_source = options.material_source.str ? (char *)copy_string(options.material_source, tctx.temp).str : NULL;
    char *material_path   = (char *)copy_string(options.material_path.str ? options.material_path : S("material.slang"), tctx.temp).str;

    
    if ( !shader_session_create(compiler, debug, session.writeRef()) ) {
        return false;
    }


    // Load module
    IModule *module = session->loadModuleFromSourceString(path, path, source, diagnostics.writeRef());
    log_diagnostics(diagnostics);
    if (!module) {
        log_error(S("ISession::loadModuleFromSourceString failed."));
        return false;
    }


    // Material module
    IModule *material_module = NULL;
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
        Slang::ComPtr<IEntryPoint> candidate;
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
        IComponentType *components[3];
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
    if ( material_module )
    {
        TypeReflection *type = linked->getLayout()->findTypeByName("Material");
        if ( !type ) {
            log_error(S("Failed to find type 'Material' in %s"), material_path);
            return false;
        }

        TypeLayoutReflection *layout = session->getTypeLayout(type);
        if ( !layout ) {
            log_error(S("Failed to get layout of 'Material' in %s"), material_path);
            return false;
        }

        // Reflect each field in the material
        u32 num_fields = layout->getFieldCount();

        Array<Shader_Field> fields = {};
        fields.allocator = tctx.temp;

        for (u32 i = 0; i < num_fields; ++i) 
        {
            Shader_Field f = {};

            VariableLayoutReflection *field = layout->getFieldByIndex(i);
            TypeLayoutReflection *l = field->getTypeLayout();
            VariableReflection *var = field->getVariable();

            // Get attribute
            for (u32 a = 0; a < var->getUserAttributeCount(); ++a) {
                UserAttribute *attr = var->getUserAttributeByIndex(a);
                f.attribute = utf8c((u8*)attr->getName());
            }

            // Get type
            auto type = get_shader_field_type(l, material_path);
            if (type == SHADER_FIELD_INVALID) return false;
            f.type = type;

            // Get name
            f.name = utf8c((u8*)field->getName());

            array_add(&fields, f);
        }


        String_Builder sb = {};
        init(&sb, tctx.temp);


        String s = flush(&sb);
        log_print(s);
    }

    log_info(S("Compiled shader."));

    return true;
}

Pair<b32, Shader_Struct> shader_reflect_material(Shader_Compiler *compiler, String filepath) 
{
    using namespace slang;

    Shader_Struct result = {};

    Slang::ComPtr<slang::ISession> session;
    Slang::ComPtr<slang::IBlob>    diagnostics;

    if ( !shader_session_create(compiler, true, session.writeRef()) ) {
        return {false, result};
    }


    // Read shader source file
    String contents = read_entire_file(filepath, tctx.temp, true);
    if (!contents) {
        log_error(S("Failed to read file: '%S'"), filepath);
        return {false, result};
    }

    const char *filepath_c = (const char *)filepath.str;
    const char *source_c   = (const char *)contents.str;


    // Create module from session
    IModule *module = session->loadModuleFromSourceString(filepath_c, filepath_c, source_c, diagnostics.writeRef());
    log_diagnostics(diagnostics);
    if ( !module ) {
        log_error(S("Failed to load module from source '%S'"), filepath);
        return {false, result};
    }


    // Find for 'IMaterial' type
    ShaderReflection *shader_reflection = module->getLayout(0, diagnostics.writeRef());
    TypeReflection *imaterial = shader_reflection->findTypeByName("IMaterial");
    if ( !imaterial ) {
        log_error(S("'IMaterial' not found in '%S'. Is 'import material' missing?"), filepath);
        return {false, result};
    }


    DeclReflection *module_decl = module->getModuleReflection();
    for (u32 i = 0; i < module_decl->getChildrenCount(); ++i) {
        DeclReflection *child = module_decl->getChild(i);
        if ( child->getKind() != DeclReflection::Kind::Struct )
            continue;

        String name = utf8c((u8 *)child->getName());
        if ( name == S("Material") )
            continue;

        TypeReflection *type = child->getType();
        if ( !shader_reflection->isSubType(type, imaterial) )
            continue;

        TypeLayoutReflection *type_layout = session->getTypeLayout(type, 0, 
                                                                   LayoutRules::DefaultStructuredBuffer, 
                                                                   diagnostics.writeRef());
        if ( !type_layout ) {
            log_diagnostics(diagnostics);
            return {false, result};
        }


        // struct name
        result.name = name;


        // Fill in the fields.
        u32 num_fields = type_layout->getFieldCount();
        for (u32 i = 0; i < num_fields; ++i) {
            if (result.num_fields >= array_count(result.fields)) {
                log_error(S("Exceeded number of possible fields: '%d' in material: '%S' in file: '%S'"),
                          array_count(result.fields), name, filepath);
                return {false, result};
            }

            VariableLayoutReflection *field    = type_layout->getFieldByIndex(i);
            TypeLayoutReflection *field_layout = field->getTypeLayout();
            VariableReflection *var            = field->getVariable();

            Shader_Field *f = &result.fields[result.num_fields++];

            // name
            f->name = utf8c((u8*)field->getName());

            // type
            f->type = get_shader_field_type(field_layout, filepath_c);
            if (f->type == SHADER_FIELD_INVALID)
                return {false, result};

            // attribute
            for (u32 a = 0; a < var->getUserAttributeCount(); ++a) {
                UserAttribute *attr = var->getUserAttributeByIndex(a);
                f->attribute = utf8c((u8*)attr->getName());
                break;
            }
        }

        return {true, result};
    }

    return {false, result};
}
