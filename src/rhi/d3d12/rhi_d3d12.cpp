// Copyright Seong Woo Lee. All Rights Reserved.

// @Todo: Allocator

// Translation
//
static D3D12_HEAP_TYPE d3d12_heap_type_from_rhi_memory_type(RHI_Memory_Type type) {
    switch (type) {
        case RHI_MEMORY_GPU_ONLY:                   return D3D12_HEAP_TYPE_DEFAULT;
        case RHI_MEMORY_UPLOAD:                     return D3D12_HEAP_TYPE_UPLOAD;
        case RHI_MEMORY_READBACK:                   return D3D12_HEAP_TYPE_READBACK;
        default:                                    Assert(!"Undefined topology."); return {};
    }
}

static D3D12_COMPARISON_FUNC d3d12_comparison_func_from_rhi(RHI_Compare op) {
    switch (op) {
        case RHI_COMPARE_ALWAYS:                    return D3D12_COMPARISON_FUNC_ALWAYS;
        case RHI_COMPARE_NEVER:                     return D3D12_COMPARISON_FUNC_NEVER;
        case RHI_COMPARE_EQUAL:                     return D3D12_COMPARISON_FUNC_EQUAL;
        case RHI_COMPARE_NOT_EQUAL:                 return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RHI_COMPARE_LESS:                      return D3D12_COMPARISON_FUNC_LESS;
        case RHI_COMPARE_LESS_EQUAL:                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RHI_COMPARE_GREATER:                   return D3D12_COMPARISON_FUNC_GREATER;
        case RHI_COMPARE_GREATER_EQUAL:             return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        default:                                    Assert(!"Undefined compare op."); return {};
    }
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE d3d12_primitive_topology_type_from_rhi(RHI_Topology topology) {
    switch (topology) {
        case RHI_TOPOLOGY_TRIANGLES:                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case RHI_TOPOLOGY_LINES:                    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case RHI_TOPOLOGY_POINTS:                   return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        default:                                    Assert(!"Undefined topology."); return {};
    }
}

static D3D12_PRIMITIVE_TOPOLOGY d3d12_primitive_topology_from_rhi(RHI_Topology topology) {
    switch (topology) {
        case RHI_TOPOLOGY_TRIANGLES:                return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RHI_TOPOLOGY_LINES:                    return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case RHI_TOPOLOGY_POINTS:                   return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        default:                                    Assert(!"Undefined topology."); return {};
    }
}

static D3D12_BLEND d3d12_blend_factor_from_rhi(RHI_Blend_Factor blend_factor) {
    switch (blend_factor) {
        case RHI_BLEND_FACTOR_ZERO:                 return D3D12_BLEND_ZERO;
        case RHI_BLEND_FACTOR_ONE:                  return D3D12_BLEND_ONE;

        case RHI_BLEND_FACTOR_SRC_COLOR:            return D3D12_BLEND_SRC_COLOR;
        case RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:  return D3D12_BLEND_INV_SRC_COLOR;
        case RHI_BLEND_FACTOR_DST_COLOR:            return D3D12_BLEND_DEST_COLOR;
        case RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR:  return D3D12_BLEND_INV_DEST_COLOR;

        case RHI_BLEND_FACTOR_SRC_ALPHA:            return D3D12_BLEND_SRC_ALPHA;
        case RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:  return D3D12_BLEND_INV_SRC_ALPHA;
        case RHI_BLEND_FACTOR_DST_ALPHA:            return D3D12_BLEND_DEST_ALPHA;
        case RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:  return D3D12_BLEND_INV_DEST_ALPHA;

        default:                                    Assert(!"Undefined blend factor."); return {};
    }
}

static D3D12_BLEND_OP d3d12_blend_op_from_rhi(RHI_Blend_Op blend_op) {
    switch (blend_op) {
        case RHI_BLEND_OP_ADD:                      return D3D12_BLEND_OP_ADD;
        case RHI_BLEND_OP_SUBTRACT:                 return D3D12_BLEND_OP_SUBTRACT;
        case RHI_BLEND_OP_SUBTRACT_REVERSE:         return D3D12_BLEND_OP_REV_SUBTRACT;
        case RHI_BLEND_OP_MIN:                      return D3D12_BLEND_OP_MIN;
        case RHI_BLEND_OP_MAX:                      return D3D12_BLEND_OP_MAX;
        default:                                    Assert(!"Undefined blend op."); return {};
    }
}

static D3D12_FILL_MODE d3d12_fill_mode_from_rhi(RHI_Fill_Mode mode) {
    switch (mode) {
        case RHI_FILL_SOLID:                        return D3D12_FILL_MODE_SOLID;
        case RHI_FILL_WIREFRAME:                    return D3D12_FILL_MODE_WIREFRAME;
        default:                                    Assert(!"Undefined fill mode."); return {};
    }
}

static D3D12_TEXTURE_ADDRESS_MODE d3d12_texture_address_mode_from_rhi(RHI_Address address_mode) {
    switch (address_mode) {
        case RHI_ADDRESS_REPEAT:                    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case RHI_ADDRESS_REPEAT_MIRRORED:           return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case RHI_ADDRESS_CLAMP_TO_EDGE:             return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case RHI_ADDRESS_CLAMP_TO_BORDER:           return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        default:                                    Assert(!"Undefined address mode."); return {};
    }
}

static D3D12_FILTER d3d12_filter_from_rhi(RHI_Filter filter, bool is_compare_op) {
    D3D12_FILTER result = D3D12_FILTER_MIN_MAG_MIP_POINT;

    switch (filter) {
        case RHI_FILTER_LINEAR: {
            if (is_compare_op) {
                result = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            }
            result = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        } break;

        case RHI_FILTER_NEAREST: {
            if (is_compare_op) {
                result = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            }
            result = D3D12_FILTER_MIN_MAG_MIP_POINT;
        } break;

        default: Assert(!"Undefined filter mode.");
    }

    return result;
}


static void d3d12_log_message(D3D12_MESSAGE_SEVERITY severity, LPCSTR description) {
    String s = S("N/A");
    Log_Level level = LOG_INFO;

    switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
            s = S("Corruption");
            level = LOG_ERROR;
            break;

        case D3D12_MESSAGE_SEVERITY_ERROR:
            s = S("Error");
            level = LOG_ERROR;
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            s = S("Warning");
            level = LOG_WARNING;
            break;

        case D3D12_MESSAGE_SEVERITY_INFO:
            s = S("Info");
            level = LOG_INFO;
            break;

        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            s = S("Message");
            level = LOG_INFO;
            break;

        default:
            break;
    }

    log(level, S("%S: %s"), s, description);
}

static void d3d12_flush_messages(ID3D12InfoQueue1 *info_queue) {
    if (info_queue) {
        D3D12_MESSAGE *msg = NULL;
        u64 num = info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
        SIZE_T allocated = 0;

        for (u64 i = 0; i < num; ++i) {
            SIZE_T sz = 0;
            info_queue->GetMessage(i, NULL, &sz);

            if (sz > allocated) {
                if (msg) {
                    dealloc(msg);
                    msg = NULL;
                    allocated = 0;
                }

                msg = (D3D12_MESSAGE *)alloc(sz);
                allocated = sz;
            }

            if (msg) {
                info_queue->GetMessage(i, msg, &sz);
                d3d12_log_message(msg->Severity, msg->pDescription);
            }
        }

        if (msg) dealloc(msg);

        info_queue->ClearStoredMessages();
    }
}

static void d3d12_message_callback(D3D12_MESSAGE_CATEGORY category, 
                                   D3D12_MESSAGE_SEVERITY severity, 
                                   D3D12_MESSAGE_ID ID, 
                                   LPCSTR description, 
                                   void *context) {

    auto *device = (RHI_Device *)context;
    bool break_on_warning = device->d3d12.break_on_warning;

    switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
            if (description) d3d12_log_message(severity, description);
            __debugbreak();
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            if (description) d3d12_log_message(severity, description);
            if (break_on_warning) __debugbreak();
            break;

        default:
            break;
    }
}

static D3D12_COMMAND_LIST_TYPE d3d12_translate_queue_type(RHI_Command_Type type) {
    static_assert(RHI_COMMAND_TYPE_COUNT == 3);
    switch (type) {
        case RHI_COMMAND_TYPE_GRAPHICS:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;

        case RHI_COMMAND_TYPE_COMPUTE:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;

        case RHI_COMMAND_TYPE_TRANSFER:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default: 
            return D3D12_COMMAND_LIST_TYPE_NONE;
    }
}

static bool d3d12_queue_init(D3D12_Device *device, D3D12_Command_Queue *queue, D3D12_COMMAND_LIST_TYPE type) {
    D3D12_COMMAND_QUEUE_DESC desc = {};
    {
        desc.Type     = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags    = {};
        desc.NodeMask = NODE_MASK;
    }

    HRESULT hr = device->device_0->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue->queue_0));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %S, %x. ID3D12Device::CreateCommandQueue failed."), string_from_hresult(hr), hr);
        return false;
    }

    queue->type = type;
    log(LOG_INFO, S("Initialized d3d12 command queue."));
    return true;
}

static void d3d12_queue_deinit(D3D12_Command_Queue *queue) {
    if (queue) {
        COM_SAFE_RELEASE(&queue->queue_0);
    }
    log(LOG_INFO, S("Deinitialized d3d12 command queue."));
}


// Descriptor
//
static bool d3d12_descriptor_heap_init(D3D12_Device *device, 
                                       D3D12_Descriptor_Heap *heap, 
                                       D3D12_DESCRIPTOR_HEAP_TYPE type, 
                                       UINT minimum_descriptors) {
    static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES == 4);

    heap->type = type;

    D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if      (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)     flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    // Align to 64 for our free list.
    u32 allocated = align_up(minimum_descriptors, 64u);

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    {
        heap_desc.Type           = type;
        heap_desc.NumDescriptors = allocated;
        heap_desc.Flags          = flags;
        heap_desc.NodeMask       = NODE_MASK;
    }

    // Create descriptor heap.
    HRESULT hr = device->device_0->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap->heap_0));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %S, %x. ID3D12Device::CreateDescriptorHeap failed."), string_from_hresult(hr), hr);
        return false;
    }

    // Get base CPU and GPU handle.
    heap->base_cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->heap_0->GetCPUDescriptorHandleForHeapStart());
    if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        heap->base_gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->heap_0->GetGPUDescriptorHandleForHeapStart());
    }

    // Get descriptor size.
    heap->descriptor_size = device->device_0->GetDescriptorHandleIncrementSize(type);

    // Make a free list.
    u32 n = allocated / 64u;
    u64 *free_list = (u64 *)alloc(sizeof(u64) * n);
    memset(free_list, 0xff, sizeof(u64) * n);
    heap->free_list = free_list;
    heap->free_list_node_count = n;
    
    heap->allocated = allocated;
    heap->count     = 0;

    // Log
    String type_str = S("N/A");
    if      (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) type_str = S("CBV_SRV_UAV");
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)     type_str = S("SAMPLER");
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)         type_str = S("RTV");
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)         type_str = S("DSV");

    log(LOG_INFO, S("Initialized d3d12 descriptor heap, type: %S"), type_str);
    return true;
}

static void d3d12_descriptor_heap_deinit(D3D12_Descriptor_Heap *heap) {
    if (heap) {
        COM_SAFE_RELEASE(&heap->heap_0);
        dealloc(heap->free_list);
    }
}

static D3D12_Descriptor d3d12_descriptor_alloc(D3D12_Descriptor_Heap *heap) {
    u32 index = 0xffffffff;

    // @Todo: One option to make it faster is making it hierarchical.
    for (u32 i = 0; i < heap->free_list_node_count; ++i) {
        u64 bits = heap->free_list[i];
        u32 b = (u32)tzcnt64(bits);
        if (b < 64) {
            bits ^= (1ull << b);
            index = i * 64 + b;
            heap->free_list[i] = bits;
            break;
        }
    }

    // @Todo: grow?
    Assert(index != 0xffffffff);

    int offset = index * heap->descriptor_size;

    D3D12_Descriptor desc = {};
    desc.type = heap->type;
    desc.index = index;
    desc.my_heap = heap;

    desc.cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->base_cpu_handle, offset);
    auto type = heap->type;
    if      (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) desc.gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->base_gpu_handle, offset);
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)     desc.gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->base_gpu_handle, offset);

    heap->count += 1;

    return desc;
}

static void d3d12_descriptor_dealloc(D3D12_Descriptor *descriptor) {
    u64 a = descriptor->index / 64llu;
    u64 b = descriptor->index % 64llu;
    
    auto *heap = descriptor->my_heap;

    Assert((heap->free_list[a] & (1ull << b)) == 0);
    heap->free_list[a] |= (1ull << b);

    heap->count -= 1;
}

// Device
//
bool d3d12_device_init(RHI_Device *device, bool debug, bool break_on_warning) {
#if USE_PIX
    log(LOG_INFO, S("USE_PIX = %d"), USE_PIX);
#else
    log(LOG_INFO, S("USE_PIX undefined."));
#endif

    // @Todo: Cleanup on failure.
    bool result = false;
    HRESULT hr = S_OK;
    auto *d3d12 = &device->d3d12;

    IDXGIDebug       *dxgi_debug        = NULL;
    ID3D12Debug      *debug_interface   = NULL;
    ID3D12Debug5     *debug_interface_5 = NULL;
    ID3D12InfoQueue1 *info_queue_1      = NULL;
    ID3DBlob         *signature_blob    = NULL;
    ID3DBlob         *error_blob        = NULL;


    d3d12->break_on_warning = break_on_warning;

    if (debug) {
        d3d12->dxgi_debug_dll_handle = LoadLibraryW(L"dxgidebug.dll");
        if (d3d12->dxgi_debug_dll_handle) {
            typedef HRESULT(WINAPI *DXGI_Get_Debug_Interface)(REFIID, void **);
            DXGI_Get_Debug_Interface dxgi_get_debug_interface = (DXGI_Get_Debug_Interface)(void*)(GetProcAddress(d3d12->dxgi_debug_dll_handle, "DXGIGetDebugInterface"));
            if (dxgi_get_debug_interface) {
                dxgi_get_debug_interface(IID_PPV_ARGS(&dxgi_debug));
            }
        }

        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
        if (SUCCEEDED(hr)) {
            hr = debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface_5));
            if (SUCCEEDED(hr)) {
                debug_interface_5->EnableDebugLayer();
                debug_interface_5->SetEnableGPUBasedValidation(true);
                debug_interface_5->SetEnableSynchronizedCommandQueueValidation(true);

                COM_SAFE_RELEASE(&debug_interface_5);
            } else {
                log(LOG_ERROR, S("HRESULT: %x. Failed to query interface 'ID3D12Debug5'. 'Windows 10 Build 20348' is the minimum supported version."), hr);
                return false;
            }

            COM_SAFE_RELEASE(&debug_interface);
        } else {
            log(LOG_ERROR, S("HRESULT: %x. Failed to get 'ID3D12Debug' interface."), hr);
            return false;
        }
    }


    //
    // @Todo: DRED (Device Removal Extended Data)
    // (https://microsoft.github.io/DirectX-Specs/d3d/DeviceRemovedExtendedData.html)
    //


    IDXGIFactory6 *dxgi_factory_6 = NULL;
    {
        UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
        hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgi_factory_6));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %x. 'CreateDXGIFactory2' failed on 'IDXGIFactory6'. 'IDXGIFactory6' is supported from Windows 10, version 1803."), hr);
            return false;
        }
    }


    IDXGIAdapter1 *adapter_1 = NULL;
    {
        UINT adapter_index = 0;
        DXGI_GPU_PREFERENCE preference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        hr = dxgi_factory_6->EnumAdapterByGpuPreference(adapter_index, preference, IID_PPV_ARGS(&adapter_1));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %x. IDXGIFactory6::EnumAdapterByGpuPreference() failed."), hr);
            return false;
        }
    }


    ID3D12Device *device_0 = NULL;
    {
        D3D_FEATURE_LEVEL minimum_feature_level = D3D_FEATURE_LEVEL_12_0;
        hr = D3D12CreateDevice(adapter_1, minimum_feature_level, IID_PPV_ARGS(&device_0));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %x. D3D12CreateDevice() failed."), hr);
            return false;
        }
    }


    if (debug) {
        ID3D12InfoQueue *info_queue_0 = NULL;

        hr = device_0->QueryInterface(IID_PPV_ARGS(&info_queue_0));
        if (SUCCEEDED(hr)) {
            // Query InfoQueue1 to register our callback.
            hr = info_queue_0->QueryInterface(IID_PPV_ARGS(&info_queue_1));
            if (SUCCEEDED(hr)) {
                info_queue_1->RegisterMessageCallback(d3d12_message_callback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, device, &d3d12->callback_cookie);
                info_queue_1->SetMuteDebugOutput(true);
                d3d12_flush_messages(info_queue_1);

                if (d3d12->callback_cookie == 0) {
                    log(LOG_ERROR, S("ID3D12InfoQueue1::RegisterMessageCallback failed."));
                }
            } else {
                log(LOG_ERROR, S("HRESULT: %x. ID3D12InfoQueue::QueryInterface(ID3D12InfoQueue1 *) failed."), hr);

                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,    break_on_warning);
            }

            COM_SAFE_RELEASE(&info_queue_0);
        } else {
            log(LOG_ERROR, S("HRESULT: %x. ID3D12Device::QueryInterface(ID3D12InfoQueue *) failed."), hr);
        }
    }


    {
        //
        // Bindless rendering requires Shader Model 6.6 and Resource Binding Tier 3. 
        // According to the d3d12 info db, all hardware since 2016, except two Qualcomm GPUs supports these features. 
        // (https://d3d12infodb.boolka.dev/)
        // (https://github.com/microsoft/DirectX-Specs/blob/master/d3d/HLSL_SM_6_6_DynamicResources.md)
        //
        D3D12_FEATURE_DATA_SHADER_MODEL sm = {};
        sm.HighestShaderModel = D3D_SHADER_MODEL_6_6;

        device_0->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm));
        if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_6) {
            log(LOG_ERROR, S("The device doesn't support shader model 6.6."));
            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        device_0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
        if (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3) {
            log(LOG_INFO, S("Resource binding tier: %d"), options.ResourceBindingTier);
        } else {
            log(LOG_ERROR, S("The device should have resource binding tier 3 or greater."));
            return false;
        }
    }


    { // Check for enhanced barrier feature support.
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
        hr = device_0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));

        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. CheckFeatureSupport failed."), string_from_hresult(hr), hr);
            return false;
        }

        if (!options12.EnhancedBarriersSupported) {
            log(LOG_ERROR, S("Enhanced barrier is not supported on this device."));
            return false;
        }
    }


    ID3D12Device12 *device_10 = NULL;
    if (FAILED(device_0->QueryInterface(IID_PPV_ARGS(&device_10)))) {
        log(LOG_ERROR, S("QueryInterface() for ID3D12Device10 failed. Requires DirectX 12 Agility SDK 1.7 or later."));
        return false;
    }


    // Set fields.
    d3d12->dxgi_factory_6   = dxgi_factory_6;
    d3d12->device_0         = device_0;
    d3d12->device_10        = device_10;
    d3d12->info_queue_1     = info_queue_1;
    d3d12->dxgi_debug       = dxgi_debug;


    // Create RTV and DSV heap.
    // @Todo: growable.
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->rtv_heap,      D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         1024)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->dsv_heap,      D3D12_DESCRIPTOR_HEAP_TYPE_DSV,          512)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->resource_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->sampler_heap,  D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,      256)) return false;


    // Create command queues.
    for (RHI_Command_Type type = RHI_COMMAND_TYPE_GRAPHICS; type < RHI_COMMAND_TYPE_COUNT; ++type) {
        d3d12_queue_init(d3d12, &d3d12->queues[type], d3d12_translate_queue_type(type));
    }


    { // Create bindless global root signature.
        D3D12_ROOT_PARAMETER root_params[2] = {};
        {
            // b0
            root_params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            root_params[0].Constants.Num32BitValues = RHI_MAX_32BIT_PUSH_CONSTANTS;
            root_params[0].Constants.RegisterSpace  = 0;
            root_params[0].Constants.ShaderRegister = 0;

            // b1
            root_params[1].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            root_params[1].Constants.Num32BitValues = RHI_MAX_32BIT_PUSH_CONSTANTS;
            root_params[1].Constants.RegisterSpace  = 0;
            root_params[1].Constants.ShaderRegister = 1;
        }

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
        {
            root_signature_desc.NumParameters = array_count(root_params);
            root_signature_desc.pParameters   = root_params;
            root_signature_desc.Flags         = (D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | 
                                                 D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED     | 
                                                 D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        }

        hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. D3D12SerializeRootSignature failed. Error blob says %s."), string_from_hresult(hr), hr, error_blob ? error_blob->GetBufferPointer() : "none");
            return false;
        }

        hr = device->d3d12.device_10->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&device->d3d12.global_root_signature));

        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. CreateRootSignature failed."), string_from_hresult(hr), hr);
            goto lb_fail;
        }
    }

    log(LOG_INFO, S("Initialized d3d12 device."));
    result = true;

lb_fail:
    COM_SAFE_RELEASE(&signature_blob);
    COM_SAFE_RELEASE(&error_blob);
    COM_SAFE_RELEASE(&adapter_1);

    return result;
}

void d3d12_device_deinit(RHI_Device *device) {
    Assert(device->kind == RHI_KIND_D3D12);

    D3D12_Device *d = &device->d3d12;

    // Destroy command queues.
    for (RHI_Command_Type type = RHI_COMMAND_TYPE_GRAPHICS; type < RHI_COMMAND_TYPE_COUNT; ++type) {
        d3d12_queue_deinit(&d->queues[type]);
    }

    // Destroy heaps.
    d3d12_descriptor_heap_deinit(&d->rtv_heap);
    d3d12_descriptor_heap_deinit(&d->dsv_heap);
    d3d12_descriptor_heap_deinit(&d->resource_heap);
    d3d12_descriptor_heap_deinit(&d->sampler_heap);

    COM_SAFE_RELEASE(&d->dxgi_factory_6);
    COM_SAFE_RELEASE(&d->device_10);
    COM_SAFE_RELEASE(&d->device_0);

    if (d->info_queue_1 && d->callback_cookie != 0) {
        d->info_queue_1->UnregisterMessageCallback(d->callback_cookie);
        d->callback_cookie = 0;
    }

    if (d->dxgi_debug) {
        // @Todo: Since info_queue_1 isn't released, there's a log saying there's a leak.
        d->dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        COM_SAFE_RELEASE(&d->dxgi_debug);
    }

    if (d->info_queue_1) {
        d3d12_flush_messages(d->info_queue_1);
        COM_SAFE_RELEASE(&d->info_queue_1);
    }


    COM_SAFE_RELEASE(&d->global_root_signature);


    if (d->dxgi_debug_dll_handle) {
        FreeLibrary(d->dxgi_debug_dll_handle);
        d->dxgi_debug_dll_handle = {};
    }


    log(LOG_INFO, S("Deinitialized d3d12 device"));
}

// Command List
//
bool d3d12_command_list_init(RHI_Device *device, RHI_Command_Buffer *cmd_buffer, RHI_Command_Type type) {
    auto native_type = d3d12_translate_queue_type(type);
    cmd_buffer->my_device = device;

    HRESULT hr = device->d3d12.device_10->CreateCommandAllocator(native_type, IID_PPV_ARGS(&cmd_buffer->d3d12.allocator));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %x. CreateCommandAllocator failed."), hr);
        return false;
    }

    hr = device->d3d12.device_10->CreateCommandList1(NODE_MASK, native_type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmd_buffer->d3d12.list_0));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %x. CreateCommandList1 failed."), hr);
        return false;
    }

    hr = cmd_buffer->d3d12.list_0->QueryInterface(IID_PPV_ARGS(&cmd_buffer->d3d12.list_7));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %x. Failed to query interface for ID3D12GraphicsCommandList7. Requires the DirectX 12 Agility SDK 1.7 or later."), hr);
        return false;
    }

    String type_str = S("N/A");
    if (type == RHI_COMMAND_TYPE_GRAPHICS) type_str = S("GRAPHICS");
    if (type == RHI_COMMAND_TYPE_COMPUTE)  type_str = S("COMPUTE");
    if (type == RHI_COMMAND_TYPE_TRANSFER) type_str = S("TRANSFER");

    log(LOG_INFO, S("Initialized d3d12 command list, type: %S"), type_str);
    return true;
}

void d3d12_command_list_deinit(RHI_Command_Buffer *cmd_buffer) {
    if (cmd_buffer) {
        cmd_buffer->my_device = NULL;
        COM_SAFE_RELEASE(&cmd_buffer->d3d12.list_0);
        COM_SAFE_RELEASE(&cmd_buffer->d3d12.list_7);
        COM_SAFE_RELEASE(&cmd_buffer->d3d12.allocator);
    }
}

void d3d12_command_list_begin(RHI_Command_Buffer *cmd_buffer) {
    // Although an ID3D12GraphicsCommandList can be reset immediately after
    // execution (provided it is associated with a different allocator, or the
    // current allocator is no longer in use by the GPU), our abstraction keeps a
    // 1:1 relationship between a command list and its allocator.
    //
    // Therefore, the caller must ensure the GPU has finished executing commands
    // recorded with this allocator before calling d3d12_command_list_begin().
    //
    auto *device = &cmd_buffer->my_device->d3d12;
    auto *list   = &cmd_buffer->d3d12;

    if (!list->is_recording) {
        list->allocator->Reset();
        list->list_7->Reset(list->allocator, NULL);
        list->is_recording = true;
    } else {
        log(LOG_WARNING, S("Command list wasn't closed."));
    }

    // [0] for resource, [1] for sampler.
    if (cmd_buffer->type != RHI_COMMAND_TYPE_TRANSFER) {
        ID3D12DescriptorHeap* heaps[] = { device->resource_heap.heap_0, device->sampler_heap.heap_0 };
        list->list_7->SetDescriptorHeaps(2, heaps);
    }
}

void d3d12_command_list_end(D3D12_Command_List *list) {
    if (list->is_recording) {
        list->list_7->Close();
        list->is_recording = false;
    } else {
        log(LOG_WARNING, S("Command list was already closed."));
    }
}

void d3d12_submit(RHI_Device *device, u32 count, RHI_Command_Buffer **cmd_buffer) { 
    auto **cmd_lists = (ID3D12CommandList **)alloc(sizeof(ID3D12CommandList *)*count, tctx.temp);

    // @Todo: Is it slow?
    for (u32 i = 0; i < count; ++i) {
        auto *queue = device->d3d12.queues[cmd_buffer[i]->type].queue_0;
        queue->ExecuteCommandLists(1, (ID3D12CommandList **)&cmd_buffer[i]->d3d12.list_7);
    }
}

// Render Pass
//
void d3d12_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Pass *pass) {
#if USE_PIX
    PIXBeginEvent(cmd_buffer->d3d12.list_7, PIX_COLOR_INDEX(0), (const char *)pass->name.str);
#endif

    auto *list = cmd_buffer->d3d12.list_7;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handles[RHI_MAX_COLOR_ATTACHMENTS] = {};

    Assert(pass->num_color_attachments <= RHI_MAX_COLOR_ATTACHMENTS);

    for (u32 i = 0; i < pass->num_color_attachments; ++i) {
        auto attachment = pass->color_attachments[i]; 
        rtv_handles[i] = attachment.view.d3d12.cpu_handle;

        if (attachment.load_op == RHI_LOAD_OP_CLEAR) {
            list->ClearRenderTargetView(rtv_handles[i], attachment.clear_color, 0, NULL);
        }
    }

    if (pass->has_depth_attachment) {
        dsv_handle = pass->depth_attachment.view.d3d12.cpu_handle;
        if (pass->depth_attachment.load_op == RHI_LOAD_OP_CLEAR) {
            list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, pass->depth_attachment.clear_depth, 0, 0, NULL);
        }
    }

    list->OMSetRenderTargets(pass->num_color_attachments, rtv_handles, FALSE, pass->has_depth_attachment ? &dsv_handle : NULL);
}

void d3d12_pass_end(RHI_Command_Buffer *cmd_buffer, RHI_Pass *pass) {
#if USE_PIX
    PIXEndEvent(cmd_buffer->d3d12.list_7);
#endif
}

// Texture
//
static DXGI_FORMAT dxgi_format_from_rhi(RHI_Format format) {
    switch (format) {
        case RHI_FORMAT_UNKNOWN:
            return DXGI_FORMAT_UNKNOWN;

        case RHI_FORMAT_R8_UNORM:
            return DXGI_FORMAT_R8_UNORM;

        case RHI_FORMAT_RG8_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;

        case RHI_FORMAT_RGBA8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case RHI_FORMAT_BGRA8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case RHI_FORMAT_RGBA8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case RHI_FORMAT_BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case RHI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        case RHI_FORMAT_RG16_UNORM:
            return DXGI_FORMAT_R16G16_UNORM;

        case RHI_FORMAT_RGBA16_UNORM:
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case RHI_FORMAT_R16F:
            return DXGI_FORMAT_R16_FLOAT;

        case RHI_FORMAT_RG16F:
            return DXGI_FORMAT_R16G16_FLOAT;

        case RHI_FORMAT_RGBA16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case RHI_FORMAT_R32F:
            return DXGI_FORMAT_R32_FLOAT;

        case RHI_FORMAT_RG32F:
            return DXGI_FORMAT_R32G32_FLOAT;

        case RHI_FORMAT_RGBA32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case RHI_FORMAT_D32F:
            return DXGI_FORMAT_D32_FLOAT;

        case RHI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;

        case RHI_FORMAT_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case RHI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;

        case RHI_FORMAT_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case RHI_FORMAT_BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;

        case RHI_FORMAT_BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;

        case RHI_FORMAT_BC6H_UFLOAT:
            return DXGI_FORMAT_BC6H_UF16;

        case RHI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;

        case RHI_FORMAT_BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            Assert(!"Unknown texture format.");
            return DXGI_FORMAT_UNKNOWN;
    }
}

static RHI_Format rhi_texture_format_from_d3d12(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_UNKNOWN:
            return RHI_FORMAT_UNKNOWN;

        case DXGI_FORMAT_R8_UNORM:
            return RHI_FORMAT_R8_UNORM;

        case DXGI_FORMAT_R8G8_UNORM:
            return RHI_FORMAT_RG8_UNORM;

        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return RHI_FORMAT_RGBA8_UNORM;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return RHI_FORMAT_BGRA8_UNORM;

        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return RHI_FORMAT_RGBA8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return RHI_FORMAT_BGRA8_UNORM_SRGB;

        case DXGI_FORMAT_R16_UNORM:
            return RHI_FORMAT_R16_UNORM;

        case DXGI_FORMAT_R16G16_UNORM:
            return RHI_FORMAT_RG16_UNORM;

        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return RHI_FORMAT_RGBA16_UNORM;

        case DXGI_FORMAT_R16_FLOAT:
            return RHI_FORMAT_R16F;

        case DXGI_FORMAT_R16G16_FLOAT:
            return RHI_FORMAT_RG16F;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return RHI_FORMAT_RGBA16F;

        case DXGI_FORMAT_R32_FLOAT:
            return RHI_FORMAT_R32F;

        case DXGI_FORMAT_R32G32_FLOAT:
            return RHI_FORMAT_RG32F;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return RHI_FORMAT_RGBA32F;

        case DXGI_FORMAT_D32_FLOAT:
            return RHI_FORMAT_D32F;

        case DXGI_FORMAT_BC1_UNORM:
            return RHI_FORMAT_BC1_UNORM;

        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return RHI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC3_UNORM:
            return RHI_FORMAT_BC3_UNORM;

        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return RHI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_BC4_UNORM:
            return RHI_FORMAT_BC4_UNORM;

        case DXGI_FORMAT_BC5_UNORM:
            return RHI_FORMAT_BC5_UNORM;

        case DXGI_FORMAT_BC6H_UF16:
            return RHI_FORMAT_BC6H_UFLOAT;

        case DXGI_FORMAT_BC7_UNORM:
            return RHI_FORMAT_BC7_UNORM;

        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return RHI_FORMAT_BC7_UNORM_SRGB;

        default:
            Assert(!"Unknown D3D12 texture format.");
            return RHI_FORMAT_UNKNOWN;
    }
}

static D3D12_RESOURCE_DIMENSION d3d12_resource_dimension_from_rhi_texture_type(RHI_Texture_Type type) {
    switch (type) {
        case RHI_TEXTURE_TYPE_1D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

        case RHI_TEXTURE_TYPE_2D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        case RHI_TEXTURE_TYPE_2D_ARRAY:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        case RHI_TEXTURE_TYPE_3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

        case RHI_TEXTURE_TYPE_CUBE:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        default:
            Assert(!"Unknown texture type.");
            return D3D12_RESOURCE_DIMENSION_UNKNOWN;
    }
}

static D3D12_RESOURCE_FLAGS d3d12_resource_flags_from_texture_usage(RHI_Texture_Usage usage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE; // 0

    if (usage & RHI_TEXTURE_USAGE_STORAGE)                  flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if (usage & RHI_TEXTURE_USAGE_COLOR_ATTACHMENT)         flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (usage & RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    return flags;
}

static D3D12_SHADER_RESOURCE_VIEW_DESC d3d12_srv_desc_from_rhi_buffer_desc(RHI_Buffer_View_Desc *desc) {
    D3D12_SHADER_RESOURCE_VIEW_DESC result = {};
    result.ViewDimension            = D3D12_SRV_DIMENSION_BUFFER;
    result.Shader4ComponentMapping  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc->type == RHI_BUFFER_VIEW_TYPE_RAW) {
        result.Format = DXGI_FORMAT_R32_TYPELESS;
        result.Buffer.FirstElement        = desc->offset / 4;
        result.Buffer.NumElements         = desc->size   / 4;
        result.Buffer.StructureByteStride = 0;
        result.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW ;
    } else if (desc->type == RHI_BUFFER_VIEW_TYPE_STRUCTURED) {
        result.Format = DXGI_FORMAT_UNKNOWN;
        result.Buffer.FirstElement        = desc->offset / desc->stride;
        result.Buffer.NumElements         = desc->size   / desc->stride;
        result.Buffer.StructureByteStride = desc->stride;
        result.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
    } else {
        Assert(!"Invalid buffer view type.");
    }

    return result;
}

static D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12_uav_desc_from_rhi_buffer_desc(RHI_Buffer_View_Desc *desc) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC result = {};
    result.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

    if (desc->type == RHI_BUFFER_VIEW_TYPE_RAW) {
        result.Format                      = DXGI_FORMAT_R32_TYPELESS;
        result.Buffer.FirstElement         = desc->offset / 4;
        result.Buffer.NumElements          = desc->size   / 4;
        result.Buffer.StructureByteStride  = 0;
        result.Buffer.CounterOffsetInBytes = 0;
        result.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_RAW;
    } else if (desc->type == RHI_BUFFER_VIEW_TYPE_STRUCTURED) {
        result.Format                      = DXGI_FORMAT_UNKNOWN;
        result.Buffer.FirstElement         = desc->offset / desc->stride;
        result.Buffer.NumElements          = desc->size   / desc->stride;
        result.Buffer.StructureByteStride  = desc->stride;
        result.Buffer.CounterOffsetInBytes = 0;
        result.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;
    } else {
        Assert(!"Invalid buffer view type.");
    }

    return result;
}

static D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12_cbv_desc_from_rhi_buffer_desc(RHI_Buffer_View_Desc *desc, RHI_Buffer *buffer) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC result = {};
    result.BufferLocation = buffer->d3d12.resource->GetGPUVirtualAddress() + desc->offset;
    result.SizeInBytes    = desc->size;
    return result;
}

static D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12_uav_desc(RHI_Texture_View_Desc *desc) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC result = {};
    result.Format = dxgi_format_from_rhi(desc->format);

    switch (desc->dimension) {
        case RHI_TEXTURE_TYPE_1D: {
            auto *t = &result.Texture1D;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE1D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D: {
            auto *t = &result.Texture2D;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE2D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D_ARRAY: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_3D: {
            // @Study: Not sure of WSlice and WSize.
            auto *t = &result.Texture3D;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE3D;
            t->MipSlice             = desc->base_mip_level;
            t->FirstWSlice          = desc->base_array_layer;
            t->WSize                = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_CUBE: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        default: {
            Assert(!"Unknown texture dimension.");
        } break;
    }

    return result;
}

static D3D12_RENDER_TARGET_VIEW_DESC d3d12_rtv_desc(RHI_Texture_View_Desc *desc) {
    D3D12_RENDER_TARGET_VIEW_DESC result = {};
    result.Format = dxgi_format_from_rhi(desc->format);

    switch (desc->dimension) {
        case RHI_TEXTURE_TYPE_1D: {
            auto *t = &result.Texture1D;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE1D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D: {
            auto *t = &result.Texture2D;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE2D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D_ARRAY: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_3D: {
            // @Study: Not sure of WSlice and WSize.
            auto *t = &result.Texture3D;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE3D;
            t->MipSlice             = desc->base_mip_level;
            t->FirstWSlice          = desc->base_array_layer;
            t->WSize                = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_CUBE: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        default: {
            Assert(!"Unknown texture dimension.");
        } break;
    }

    return result;
}

static D3D12_DEPTH_STENCIL_VIEW_DESC d3d12_dsv_desc(RHI_Texture_View_Desc *desc) {
    D3D12_DEPTH_STENCIL_VIEW_DESC result = {};
    result.Format = dxgi_format_from_rhi(desc->format);

    switch (desc->dimension) {
        case RHI_TEXTURE_TYPE_1D: {
            auto *t = &result.Texture1D;
            result.ViewDimension    = D3D12_DSV_DIMENSION_TEXTURE1D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D: {
            auto *t = &result.Texture2D;
            result.ViewDimension    = D3D12_DSV_DIMENSION_TEXTURE2D;
            t->MipSlice             = desc->base_mip_level;
        } break;

        case RHI_TEXTURE_TYPE_2D_ARRAY: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        default: {
            Assert(!"Invalid texture dimension.");
        } break;
    }

    return result;
}


bool d3d12_buffer_init(RHI_Device *device, RHI_Buffer *buffer, RHI_Buffer_Desc *desc, RHI_Heap *heap) {
    if (heap) {
        Assert(!"Not implemented at the moment.");
    } else {
        D3D12_HEAP_PROPERTIES heap_prop = {};
        heap_prop.Type = d3d12_heap_type_from_rhi_memory_type(desc->memory_type);

        D3D12_CLEAR_VALUE *clear_value = NULL;

        D3D12_RESOURCE_DESC resource_desc = {};
        {
            resource_desc.Dimension         = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment         = 0; // effectively 64KB
            resource_desc.Width             = desc->size;
            resource_desc.Height            = 1;
            resource_desc.DepthOrArraySize  = 1;
            resource_desc.MipLevels         = 1;
            resource_desc.Format            = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc        = { 1, 0 };
            resource_desc.Layout            = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            // @Study: Is setting flags in buffers expensive?
            resource_desc.Flags = (desc->memory_type == RHI_MEMORY_UPLOAD || desc->memory_type == RHI_MEMORY_READBACK ?
                                   D3D12_RESOURCE_FLAG_NONE :
                                   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        }

        HRESULT hr = device->d3d12.device_10->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE,
                                                                      &resource_desc, D3D12_RESOURCE_STATE_COMMON,
                                                                      clear_value, IID_PPV_ARGS(&buffer->d3d12.resource));

        if (FAILED(hr)) {
            log(LOG_ERROR, S("ID3D12Device::CreateCommittedResource failed."));
            return false;
        }
    }

    log(LOG_INFO, S("Initialized d3d12 buffer."));
    return true;
}

void d3d12_buffer_deinit(RHI_Buffer *buffer) {
    if (buffer) {
        COM_SAFE_RELEASE(&buffer->d3d12.resource);
    }
    log(LOG_INFO, S("Deinitialized d3d12 buffer."));
}

void *d3d12_buffer_map(RHI_Buffer *buffer) {
    void *ptr;
    D3D12_RANGE read_range = { 0, 0 }; // No read.
    buffer->d3d12.resource->Map(0, &read_range, &ptr);
    return ptr;
}

void d3d12_buffer_unmap(RHI_Buffer *buffer) {
    D3D12_RANGE written_range = { 0, buffer->desc.size };
    buffer->d3d12.resource->Unmap(0, &written_range);
}

void d3d12_buffer_view_init(RHI_Device *device, RHI_Buffer_View *view, RHI_Buffer *buffer, RHI_Buffer_View_Desc *desc) {
    ID3D12Resource *resource = buffer->d3d12.resource;

    view->d3d12    = d3d12_descriptor_alloc(&device->d3d12.resource_heap);
    view->bindless = view->d3d12.index;

    if (desc->type == RHI_BUFFER_VIEW_TYPE_CONSTANT) {
        auto cbv_desc = d3d12_cbv_desc_from_rhi_buffer_desc(desc, buffer);
        device->d3d12.device_10->CreateConstantBufferView(&cbv_desc, view->d3d12.cpu_handle);
    } else {
        if (desc->writable) {
            auto uav_desc = d3d12_uav_desc_from_rhi_buffer_desc(desc);
            device->d3d12.device_10->CreateUnorderedAccessView(resource, NULL, &uav_desc, view->d3d12.cpu_handle);
        } else {
            auto srv_desc = d3d12_srv_desc_from_rhi_buffer_desc(desc);
            device->d3d12.device_10->CreateShaderResourceView(resource, &srv_desc, view->d3d12.cpu_handle);
        }
    }

    log(LOG_INFO, S("Initialized d3d12 buffer view."));
}

void d3d12_buffer_view_deinit(RHI_Buffer_View *view) {
    d3d12_descriptor_dealloc(&view->d3d12);
    memset(view, 0, sizeof(*view));
    log(LOG_INFO, S("Deinitialized d3d12 buffer view."));
}

bool d3d12_texture_init(RHI_Device *device, RHI_Texture *texture, RHI_Texture_Desc *desc, RHI_Heap *heap) {
    D3D12_RESOURCE_DESC resource_desc = {};
    {
        resource_desc.Dimension         = d3d12_resource_dimension_from_rhi_texture_type(desc->type);
        resource_desc.Alignment         = 0; // effectively 64KB.
        resource_desc.Width             = desc->width;
        resource_desc.Height            = desc->height;
        resource_desc.DepthOrArraySize  = desc->depth;
        resource_desc.MipLevels         = desc->mip_levels;
        resource_desc.Format            = dxgi_format_from_rhi(desc->format);
        resource_desc.SampleDesc        = { 1, 0 };
        resource_desc.Layout            = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resource_desc.Flags             = d3d12_resource_flags_from_texture_usage(desc->usage);
    }

    if (heap) {
        Assert(!"User heap not supported at the moment.");
    } else {
        D3D12_HEAP_PROPERTIES heap_prop = {};
        heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE clear = {};

        if (desc->clear) {
            clear.Format = resource_desc.Format;
            if (clear.Format == DXGI_FORMAT_D32_FLOAT) {
                clear.DepthStencil.Depth = desc->clear_depth;
            } else if (clear.Format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
                clear.DepthStencil.Depth = desc->clear_depth;
                clear.DepthStencil.Stencil = desc->clear_stencil;
            } else {
                memcpy(&clear.Color, desc->clear_color, 16);
            }
        }

        HRESULT hr = device->d3d12.device_10->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, 
                                                                      &resource_desc, D3D12_RESOURCE_STATE_COMMON, 
                                                                      desc->clear ? &clear : NULL, IID_PPV_ARGS(&texture->d3d12.resource));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. CreateCommittedResource failed."), string_from_hresult(hr), hr);
            return false;
        }
    }

    log(LOG_INFO, S("Initialized d3d12 texture."));
    return true;
}

void d3d12_texture_deinit(RHI_Texture *texture) {
    // Make sure the texture isn't in flight!
    COM_SAFE_RELEASE(&texture->d3d12.resource);
    log(LOG_INFO, S("Deinitialized d3d12 texture."));
}

static D3D12_SHADER_RESOURCE_VIEW_DESC d3d12_srv_desc(RHI_Texture_View_Desc *desc) {
    D3D12_SHADER_RESOURCE_VIEW_DESC result = {};
    result.Format = dxgi_format_from_rhi(desc->format);
    result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (desc->dimension) {
        case RHI_TEXTURE_TYPE_1D: {
            auto *t = &result.Texture1D;
            result.ViewDimension    = D3D12_SRV_DIMENSION_TEXTURE1D;
            t->MostDetailedMip      = desc->base_mip_level;
            t->MipLevels            = desc->mip_levels;
        } break;

        case RHI_TEXTURE_TYPE_2D: {
            auto *t = &result.Texture2D;
            result.ViewDimension    = D3D12_SRV_DIMENSION_TEXTURE2D;
            t->MostDetailedMip      = desc->base_mip_level;
            t->MipLevels            = desc->mip_levels;
        } break;

        case RHI_TEXTURE_TYPE_2D_ARRAY: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            t->MostDetailedMip      = desc->base_mip_level;
            t->MipLevels            = desc->mip_levels;
            t->FirstArraySlice      = desc->base_array_layer;
            t->ArraySize            = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_3D: {
            auto *t = &result.Texture3D;
            result.ViewDimension    = D3D12_SRV_DIMENSION_TEXTURE3D;
            t->MostDetailedMip      = desc->base_mip_level;
            t->MipLevels            = desc->mip_levels;
        } break;

        case RHI_TEXTURE_TYPE_CUBE: {
            auto *t = &result.TextureCube;
            result.ViewDimension    = D3D12_SRV_DIMENSION_TEXTURECUBE;
            t->MostDetailedMip      = desc->base_mip_level;
            t->MipLevels            = desc->mip_levels;
        } break;

        default: {
            Assert(!"Unknown texture dimension.");
        } break;
    }

    return result;
}

void d3d12_texture_view_init(RHI_Device *device, RHI_Texture_View *view, RHI_Texture *texture, RHI_Texture_View_Desc *desc) {
    ID3D12Resource *resource = texture->d3d12.resource;

    switch (desc->type) {
        case RHI_TEXTURE_VIEW_TYPE_SAMPLED: {
            if (texture->desc.usage & RHI_TEXTURE_USAGE_SAMPLED) {
                view->d3d12 = d3d12_descriptor_alloc(&device->d3d12.resource_heap);
                auto srv_desc = d3d12_srv_desc(desc);
                device->d3d12.device_10->CreateShaderResourceView(resource, &srv_desc, view->d3d12.cpu_handle);
            } else {
                Assert(!"Texture doesn't have a sampled usage flag.");
            }
        } break;

        case RHI_TEXTURE_VIEW_TYPE_UNORDERED_ACCESS: {
            if (texture->desc.usage & RHI_TEXTURE_USAGE_STORAGE) {
                view->d3d12 = d3d12_descriptor_alloc(&device->d3d12.resource_heap);
                auto uav_desc = d3d12_uav_desc(desc);
                device->d3d12.device_10->CreateUnorderedAccessView(resource, NULL, &uav_desc, view->d3d12.cpu_handle);
            } else {
                Assert(!"Texture doesn't have a storage usage flag.");
            }
        } break;

        case RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET: {
            if (texture->desc.usage & RHI_TEXTURE_USAGE_COLOR_ATTACHMENT) {
                view->d3d12 = d3d12_descriptor_alloc(&device->d3d12.rtv_heap);
                auto rtv_desc = d3d12_rtv_desc(desc);
                device->d3d12.device_10->CreateRenderTargetView(resource, &rtv_desc, view->d3d12.cpu_handle);
            } else {
                Assert(!"Texture doesn't have a color attachment usage flag.");
            }
        } break;

        case RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL: {
            if (texture->desc.usage & RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT) {
                view->d3d12 = d3d12_descriptor_alloc(&device->d3d12.dsv_heap);
                auto dsv_desc = d3d12_dsv_desc(desc);
                device->d3d12.device_10->CreateDepthStencilView(resource, &dsv_desc, view->d3d12.cpu_handle);
            } else {
                Assert(!"Texture doesn't have a depth stencil usage flag.");
            }
        } break;

        default:
            Assert(!"Unknown view type.");
    }

    view->bindless = view->d3d12.index;
}

void d3d12_texture_view_deinit(RHI_Texture_View *view) {
    d3d12_descriptor_dealloc(&view->d3d12);
    memset(view, 0, sizeof(*view));
}

void d3d12_sampler_init(RHI_Device *device, RHI_Sampler *sampler, RHI_Sampler_Desc *desc) {
    sampler->d3d12    = d3d12_descriptor_alloc(&device->d3d12.sampler_heap);
    sampler->bindless = sampler->d3d12.index;

    D3D12_SAMPLER_DESC sampler_desc = {};
    {
        // @Study: Shadow map filtering and compare function.
        bool is_compare = desc->compare_op != RHI_COMPARE_ALWAYS;
        sampler_desc.Filter         = d3d12_filter_from_rhi(desc->filter, is_compare);
        sampler_desc.AddressU       = d3d12_texture_address_mode_from_rhi(desc->address_u);
        sampler_desc.AddressV       = d3d12_texture_address_mode_from_rhi(desc->address_v);
        sampler_desc.AddressW       = d3d12_texture_address_mode_from_rhi(desc->address_w);
        sampler_desc.MipLODBias     = desc->mip_lod_bias;
        sampler_desc.MaxAnisotropy  = desc->max_anisotropy;
        sampler_desc.ComparisonFunc = is_compare ? d3d12_comparison_func_from_rhi(desc->compare_op) : D3D12_COMPARISON_FUNC_NONE;
        sampler_desc.BorderColor[0] = desc->border_color[0];
        sampler_desc.BorderColor[1] = desc->border_color[1];
        sampler_desc.BorderColor[2] = desc->border_color[2];
        sampler_desc.BorderColor[3] = desc->border_color[3];
        sampler_desc.MinLOD         = desc->min_lod;
        sampler_desc.MaxLOD         = desc->max_lod;
    }
    device->d3d12.device_10->CreateSampler(&sampler_desc, sampler->d3d12.cpu_handle);

    log(LOG_INFO, S("Initialized d3d12 sampler."));
}

void d3d12_sampler_deinit(RHI_Sampler *sampler) {
    d3d12_descriptor_dealloc(&sampler->d3d12);
    sampler->bindless = 0;

    log(LOG_INFO, S("Deinitialized d3d12 sampler."));
}

// Surface
//
bool d3d12_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc *desc) {
    HWND hwnd = (HWND)desc->native_window_handle;

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM; // @Todo: HDR

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    {
        swap_chain_desc.Width  = desc->width;
        swap_chain_desc.Height = desc->height;

        swap_chain_desc.Format = format;

        swap_chain_desc.Stereo = RHI_D3D12_SWAP_CHAIN_STEREO;

        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swap_chain_desc.BufferCount = desc->num_back_buffers,
        swap_chain_desc.Scaling     = DXGI_SCALING_STRETCH;

        // FLIP_DISCARD discards "old flips" in the queue and presents only the "new" flip.
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 

        // FLIP presentation model does not allow MSAA framebuffer.
        // If you want MSAA then you'll need to render offscreen and manually
        // resolve to non-MSAA framebuffer.
        swap_chain_desc.SampleDesc = { 1, 0 },

        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        // @Todo
        swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        if (desc->frame_latency_waitable)  swap_chain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc = NULL;
    IDXGIOutput *monitor = NULL;

    IDXGISwapChain1 *swap_chain_1 = NULL;

    HRESULT hr = device->d3d12.dxgi_factory_6->CreateSwapChainForHwnd(device->d3d12.queues[RHI_COMMAND_TYPE_GRAPHICS].queue_0,
                                                                      hwnd, &swap_chain_desc, fullscreen_desc,  
                                                                      monitor, &swap_chain_1);

    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %S, %x. IDXGIFactory6::CreateSwapChainForHwnd failed."), string_from_hresult(hr), hr);
        return false;
    }

    hr = swap_chain_1->QueryInterface(IID_PPV_ARGS(&surface->d3d12.swap_chain_4));
    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %S, %x. QueryInterface(IDXGISwapChain4 *) failed."), string_from_hresult(hr), hr);
        return false;
    }

    // Disable Alt + Enter changing monitor resolution to match window size.
    device->d3d12.dxgi_factory_6->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);


    // Get resources from the swap chain and create render target views.
    for (u32 i = 0; i < desc->num_back_buffers; ++i) {
        auto *tex = &surface->textures[i];

        hr = surface->d3d12.swap_chain_4->GetBuffer(i, IID_PPV_ARGS(&tex->d3d12.resource));
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. IDXGISwapChain1::GetBuffer() failed."), string_from_hresult(hr), hr);
            return false;
        }

        tex->kind = RHI_KIND_D3D12;
        tex->desc.type       = RHI_TEXTURE_TYPE_2D;
        tex->desc.format     = rhi_texture_format_from_d3d12(format);
        tex->desc.usage      = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT;
        tex->desc.width      = desc->width;
        tex->desc.height     = desc->height;
        tex->desc.mip_levels = 1;
        tex->desc.depth      = 1;
    }

    // Get initial back buffer index.
    surface->current_frame_index = surface->d3d12.swap_chain_4->GetCurrentBackBufferIndex();

    if (desc->frame_latency_waitable) {
        // Get frame latency waitable object
        surface->d3d12.frame_waitable_object = surface->d3d12.swap_chain_4->GetFrameLatencyWaitableObject();

        // It's basically setting the present queue capacity.
        hr = surface->d3d12.swap_chain_4->SetMaximumFrameLatency(2);
        if (FAILED(hr)) {
            log(LOG_ERROR, S("HRESULT: %S, %x. IDXGISwapChain2::SetMaximumFrameLatency() failed."), string_from_hresult(hr), hr);
            return false;
        }
    }


    log(LOG_INFO, S("Initialized d3d12 surface."));
    return true;
}

void d3d12_surface_present(RHI_Surface *surface, u32 sync_interval) {
    UINT flags = sync_interval == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0;
    surface->d3d12.swap_chain_4->Present(sync_interval, flags);
    surface->current_frame_index = surface->d3d12.swap_chain_4->GetCurrentBackBufferIndex();
}

void d3d12_surface_resize(RHI_Surface *surface, u32 width, u32 height) {
    for (u32 i = 0; i < surface->desc.num_back_buffers; i++) {
        COM_SAFE_RELEASE(&surface->textures[i].d3d12.resource);
    }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    surface->d3d12.swap_chain_4->GetDesc1(&desc);
    surface->d3d12.swap_chain_4->ResizeBuffers(surface->desc.num_back_buffers, width, height, desc.Format, desc.Flags);

    for (u32 i = 0; i < surface->desc.num_back_buffers; i++) {
        surface->d3d12.swap_chain_4->GetBuffer(i, IID_PPV_ARGS(&surface->textures[i].d3d12.resource));
        surface->textures[i].desc.width  = width;
        surface->textures[i].desc.height = height;
    }

    surface->desc.width          = width;
    surface->desc.height         = height;
    surface->current_frame_index = surface->d3d12.swap_chain_4->GetCurrentBackBufferIndex();
}

bool d3d12_surface_wait_for_waitable_object(RHI_Surface *surface) {
    if (surface->d3d12.frame_waitable_object != NULL) {
        DWORD result = WaitForSingleObjectEx(surface->d3d12.frame_waitable_object, 1000, true);
        if (result == WAIT_FAILED) {
            return false;
        }
    }
    return true;
}

// Fence
//
bool d3d12_fence_init(RHI_Device *device, RHI_Semaphore *fence) {
    UINT64 initial_value = 0;
    D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

    HRESULT hr = device->d3d12.device_10->CreateFence(initial_value, flags, IID_PPV_ARGS(&fence->d3d12.fence_0));

    if (FAILED(hr)) {
        log(LOG_ERROR, S("HRESULT: %S, %x. CreateFence failed."), string_from_hresult(hr), hr);
        return false;
    }

    fence->d3d12.event = CreateEvent(NULL, FALSE, FALSE, NULL);

    log(LOG_INFO, S("Initialized d3d12 fence."));
    return true;
}

void d3d12_fence_deinit(RHI_Semaphore *fence) {
    COM_SAFE_RELEASE(&fence->d3d12.fence_0);
    if (fence->d3d12.event) {
        CloseHandle(fence->d3d12.event);
        fence->d3d12.event = {};
    }

    log(LOG_INFO, S("Deinitialized d3d12 fence."));
}

void d3d12_fence_wait(RHI_Semaphore *fence, u64 value, s32 milliseconds) {
    DWORD timeout = (milliseconds == -1) ? INFINITE : (DWORD)milliseconds;
    if (fence->d3d12.fence_0->GetCompletedValue() < value) {
        fence->d3d12.fence_0->SetEventOnCompletion(value, fence->d3d12.event);
        WaitForSingleObject(fence->d3d12.event, timeout);
    }
}

u64 d3d12_fence_completed_value(RHI_Semaphore *fence) {
    return fence->d3d12.fence_0->GetCompletedValue();
}

// Commands
//
static D3D12_BARRIER_LAYOUT d3d12_barrier_layout_from_rhi(RHI_Resource_State state) {
    switch (state) {
        case RHI_RESOURCE_STATE_COMMON:
            return D3D12_BARRIER_LAYOUT_COMMON;

        case RHI_RESOURCE_STATE_RENDER_TARGET:
            return D3D12_BARRIER_LAYOUT_RENDER_TARGET;

        case RHI_RESOURCE_STATE_UNORDERED_ACCESS:
            return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;

        case RHI_RESOURCE_STATE_DEPTH_WRITE:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;

        case RHI_RESOURCE_STATE_DEPTH_READ:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;

        case RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE:
            return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

        case RHI_RESOURCE_STATE_COPY_DEST:
            return D3D12_BARRIER_LAYOUT_COPY_DEST;

        case RHI_RESOURCE_STATE_COPY_SOURCE:
            return D3D12_BARRIER_LAYOUT_COPY_SOURCE;

        case RHI_RESOURCE_STATE_GENERIC_READ:
            return D3D12_BARRIER_LAYOUT_GENERIC_READ;

        case RHI_RESOURCE_STATE_PRESENT:
            return D3D12_BARRIER_LAYOUT_PRESENT;

        default:
            return D3D12_BARRIER_LAYOUT_COMMON;
    }
}

static D3D12_BARRIER_SYNC d3d12_barrier_sync_from_rhi(RHI_Resource_State state) {
    switch (state) {
        case RHI_RESOURCE_STATE_COMMON:
            return D3D12_BARRIER_SYNC_ALL;

        case RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
            return D3D12_BARRIER_SYNC_ALL_SHADING;

        case RHI_RESOURCE_STATE_INDEX_BUFFER:
            return D3D12_BARRIER_SYNC_INDEX_INPUT;

        case RHI_RESOURCE_STATE_RENDER_TARGET:
            return D3D12_BARRIER_SYNC_RENDER_TARGET;

        case RHI_RESOURCE_STATE_UNORDERED_ACCESS:
            return D3D12_BARRIER_SYNC_ALL_SHADING;

        case RHI_RESOURCE_STATE_DEPTH_WRITE:
        case RHI_RESOURCE_STATE_DEPTH_READ:
            return D3D12_BARRIER_SYNC_DEPTH_STENCIL;

        case RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
            return D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;

        case RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
            return D3D12_BARRIER_SYNC_PIXEL_SHADING;

        case RHI_RESOURCE_STATE_INDIRECT_ARGUMENT:
            return D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;

        case RHI_RESOURCE_STATE_COPY_DEST:
        case RHI_RESOURCE_STATE_COPY_SOURCE:
            return D3D12_BARRIER_SYNC_COPY;

        case RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
            return D3D12_BARRIER_SYNC_RAYTRACING | D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;

        case RHI_RESOURCE_STATE_GENERIC_READ:
            return D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_INDEX_INPUT | D3D12_BARRIER_SYNC_EXECUTE_INDIRECT | D3D12_BARRIER_SYNC_COPY;

        case RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE:
            return D3D12_BARRIER_SYNC_ALL_SHADING;

        case RHI_RESOURCE_STATE_PRESENT:
            return D3D12_BARRIER_SYNC_ALL;

        default:
            return D3D12_BARRIER_SYNC_ALL;
    }
}

static D3D12_BARRIER_ACCESS d3d12_barrier_access_from_rhi(RHI_Resource_State state) {
    switch (state) {
        case RHI_RESOURCE_STATE_COMMON:
            return D3D12_BARRIER_ACCESS_COMMON;

        case RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
            return D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;

        case RHI_RESOURCE_STATE_INDEX_BUFFER:
            return D3D12_BARRIER_ACCESS_INDEX_BUFFER;

        case RHI_RESOURCE_STATE_RENDER_TARGET:
            return D3D12_BARRIER_ACCESS_RENDER_TARGET;

        case RHI_RESOURCE_STATE_UNORDERED_ACCESS:
            return D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;

        case RHI_RESOURCE_STATE_DEPTH_WRITE:
            return D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

        case RHI_RESOURCE_STATE_DEPTH_READ:
            return D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;

        case RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE:
            return D3D12_BARRIER_ACCESS_SHADER_RESOURCE;

        case RHI_RESOURCE_STATE_INDIRECT_ARGUMENT:
            return D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;

        case RHI_RESOURCE_STATE_COPY_DEST:
            return D3D12_BARRIER_ACCESS_COPY_DEST;

        case RHI_RESOURCE_STATE_COPY_SOURCE:
            return D3D12_BARRIER_ACCESS_COPY_SOURCE;

        case RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
            return D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ | D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

        case RHI_RESOURCE_STATE_GENERIC_READ:
            return D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER | D3D12_BARRIER_ACCESS_INDEX_BUFFER |
                   D3D12_BARRIER_ACCESS_SHADER_RESOURCE | D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT | D3D12_BARRIER_ACCESS_COPY_SOURCE;

        case RHI_RESOURCE_STATE_PRESENT:
            return D3D12_BARRIER_ACCESS_COMMON;

        default:
            return D3D12_BARRIER_ACCESS_COMMON;
    }
}

void d3d12_cmd_texture_barrier(RHI_Command_Buffer *cmd_buffer, RHI_Texture *texture, 
                               RHI_Resource_State before, RHI_Resource_State after,
                               u32 mip, u32 slice) {

    D3D12_TEXTURE_BARRIER barrier = {};
    {
        barrier.SyncBefore      = d3d12_barrier_sync_from_rhi(before);
        barrier.SyncAfter       = d3d12_barrier_sync_from_rhi(after);
        barrier.AccessBefore    = d3d12_barrier_access_from_rhi(before);
        barrier.AccessAfter     = d3d12_barrier_access_from_rhi(after);
        barrier.LayoutBefore    = d3d12_barrier_layout_from_rhi(before);
        barrier.LayoutAfter     = d3d12_barrier_layout_from_rhi(after);
        barrier.pResource       = texture->d3d12.resource;
        barrier.Flags           = D3D12_TEXTURE_BARRIER_FLAG_NONE;
    }

    bool all_mips   = (  mip == RHI_ALL_MIPS  ) ? true : false;
    bool all_slices = (slice == RHI_ALL_LAYERS) ? true : false;

    if (all_mips && all_slices) {
        barrier.Subresources = CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff);
    } else if (!all_mips && !all_slices) {
        barrier.Subresources = CD3DX12_BARRIER_SUBRESOURCE_RANGE(D3D12CalcSubresource(mip, slice, 0, texture->desc.mip_levels, texture->desc.depth));
    } else if (all_mips) {
        barrier.Subresources = CD3DX12_BARRIER_SUBRESOURCE_RANGE(0, texture->desc.mip_levels, slice, 1);
    } else {
        barrier.Subresources = CD3DX12_BARRIER_SUBRESOURCE_RANGE(mip, 1, 0, texture->desc.depth);
    }

    D3D12_BARRIER_GROUP group = {};
    {
        group.Type             = D3D12_BARRIER_TYPE_TEXTURE;
        group.NumBarriers      = 1;
        group.pTextureBarriers = &barrier;
    }

    cmd_buffer->d3d12.list_7->Barrier(1, &group);
}

void d3d12_cmd_set_pipeline(RHI_Command_Buffer *cmd_buffer, RHI_Pipeline *pipeline) {
	cmd_buffer->d3d12.list_7->SetGraphicsRootSignature(cmd_buffer->my_device->d3d12.global_root_signature);
    cmd_buffer->d3d12.list_7->SetPipelineState(pipeline->d3d12.state);
    cmd_buffer->d3d12.list_7->IASetPrimitiveTopology(d3d12_primitive_topology_from_rhi(pipeline->desc.topology));
}

void d3d12_cmd_set_viewport(RHI_Command_Buffer *cmd_buffer, float x, float y, float width, float height, float min_depth, float max_depth) {
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width    = width;
    viewport.Height   = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;
	cmd_buffer->d3d12.list_7->RSSetViewports(1, &viewport);
}

void d3d12_cmd_set_scissor(RHI_Command_Buffer *cmd_buffer, u32 x, u32 y, u32 width, u32 height) {
    D3D12_RECT rect = {};
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + width;
    rect.bottom = y + height;
    cmd_buffer->d3d12.list_7->RSSetScissorRects(1, &rect);
}

void d3d12_cmd_draw(RHI_Command_Buffer *cmd_buffer, u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance) {
    cmd_buffer->d3d12.list_7->DrawInstanced(num_vertices, num_instances, first_vertex, first_instance);
}

void d3d12_cmd_draw_indexed(RHI_Command_Buffer *cmd_buffer, RHI_Buffer *index_buffer, u32 index_size, u32 num_indices, u32 num_instances, u32 first_index, u32 first_vertex, u32 first_instance) {
    Assert(index_size == 2 || index_size == 4);

    D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = index_buffer->d3d12.resource->GetGPUVirtualAddress();
	ibv.Format         = index_size == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes    = index_buffer->desc.size;

    cmd_buffer->d3d12.list_7->IASetIndexBuffer(&ibv);
    cmd_buffer->d3d12.list_7->DrawIndexedInstanced(num_indices, num_instances, first_index, first_vertex, first_instance);
}

void d3d12_cmd_push_constants(RHI_Command_Buffer *cmd_buffer, u32 root_index, void *data, u32 size) {
    cmd_buffer->d3d12.list_7->SetGraphicsRoot32BitConstants(root_index, size / 4, data, 0);
}

void d3d12_cmd_copy_buffer_to_buffer(RHI_Command_Buffer *cmd_buffer, 
                                     RHI_Buffer *dst, RHI_Buffer *src, 
                                     u64 dst_offset, u64 src_offset, u64 size) {
    cmd_buffer->d3d12.list_7->CopyBufferRegion(dst->d3d12.resource, dst_offset, src->d3d12.resource, src_offset, size);
}

void d3d12_cmd_copy_buffer_to_texture(RHI_Command_Buffer *cmd_buffer, 
                                      RHI_Buffer *src, u32 src_offset, u32 src_pitch,  
                                      RHI_Texture *dst, RHI_Box *box, u32 mip, u32 layer) {

    Assert(src_pitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0);
    Assert(src_offset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0);

    u32 copy_width  = box->width;
    u32 copy_height = box->height;
    u32 copy_depth  = box->depth;

    if (rhi_is_bc_format(dst->desc.format)) {
        copy_width  = align_up(copy_width,  4);
        copy_height = align_up(copy_height, 4);
    }

    // Source
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_desc = {};
    {
        D3D12_SUBRESOURCE_FOOTPRINT layout = {};
        {
            layout.Format   = dxgi_format_from_rhi(dst->desc.format);
            layout.Width    = copy_width;
            layout.Height   = copy_height;
            layout.Depth    = copy_depth;
            layout.RowPitch = src_pitch;
        }
        placed_desc.Offset    = src_offset;
        placed_desc.Footprint = layout; 
    }

    D3D12_TEXTURE_COPY_LOCATION src_loc = {};
    src_loc.pResource        = src->d3d12.resource;
    src_loc.Type             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src_loc.PlacedFootprint  = placed_desc;


    // Destination
    UINT subresource_index = D3D12CalcSubresource(mip, layer, 0/*plane slice*/, 
                                                  dst->desc.mip_levels, dst->desc.depth);

    D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
    dst_loc.pResource        = dst->d3d12.resource;
    dst_loc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_loc.SubresourceIndex = subresource_index;

    D3D12_BOX b = {};
    b.right  = copy_width;
    b.bottom = copy_height;
    b.back   = copy_depth;

    cmd_buffer->d3d12.list_7->CopyTextureRegion(&dst_loc, box->x, box->y, box->z, &src_loc, &b);
}

void d3d12_cmd_copy_texture_to_texture(RHI_Command_Buffer *cmd_buffer, RHI_Texture *dst, RHI_Texture *src) {

}

void d3d12_queue_signal(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    auto *queue = &device->d3d12.queues[queue_type];
    queue->queue_0->Signal(semaphore->d3d12.fence_0, value);
}

void d3d12_queue_wait(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    auto *queue = &device->d3d12.queues[queue_type];
    queue->queue_0->Wait(semaphore->d3d12.fence_0, value);
}


// Pipeline
//
bool d3d12_pipeline_init(RHI_Device *device, RHI_Pipeline *pipeline, RHI_Pipeline_Desc *desc) {
    HRESULT hr = S_OK;

    pipeline->desc = *desc;

    switch (desc->type) {
        case RHI_PIPELINE_TYPE_GRAPHICS: {
            if (desc->num_color_attachments > 8) { 
                log(LOG_ERROR, S("Color attachment count must be less or equal than 8."));
                return false;
            }

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
            pipeline_desc.pRootSignature = device->d3d12.global_root_signature; // :)

            // No DS, HS, GS. They are failed experiments. No SO atm.
            pipeline_desc.VS.pShaderBytecode = desc->vs_data;
            pipeline_desc.VS.BytecodeLength  = desc->vs_size;

            pipeline_desc.PS.pShaderBytecode = desc->ps_data;
            pipeline_desc.PS.BytecodeLength  = desc->ps_size;

            { // Blend State
                pipeline_desc.BlendState.AlphaToCoverageEnable  = FALSE; 
                pipeline_desc.BlendState.IndependentBlendEnable = TRUE;

                for (u32 i = 0; i < desc->num_color_attachments; ++i) {
                    auto bs = &pipeline_desc.BlendState.RenderTarget[i];
                    {
                        bs->BlendEnable            = desc->blend_enabled[i];

                        if (desc->blend_enabled[i]) {
                            bs->SrcBlend               = d3d12_blend_factor_from_rhi(desc->blend_factor_color_src[i]);
                            bs->DestBlend              = d3d12_blend_factor_from_rhi(desc->blend_factor_color_dst[i]);
                            bs->BlendOp                = d3d12_blend_op_from_rhi(desc->blend_op_color[i]);

                            bs->SrcBlendAlpha          = d3d12_blend_factor_from_rhi(desc->blend_factor_alpha_src[i]);
                            bs->DestBlendAlpha         = d3d12_blend_factor_from_rhi(desc->blend_factor_alpha_dst[i]);
                            bs->BlendOpAlpha           = d3d12_blend_op_from_rhi(desc->blend_op_alpha[i]);
                        }

                        bs->LogicOpEnable          = FALSE;
                        bs->LogicOp                = D3D12_LOGIC_OP_NOOP;

                        bs->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                    }
                }

                pipeline_desc.SampleMask = 0xffffffff;
            }

            { // Raster State
                auto *rs = &pipeline_desc.RasterizerState;
                rs->FillMode                = d3d12_fill_mode_from_rhi(desc->fill_mode);
                rs->CullMode                = (desc->cull_mode != RHI_CULL_NONE) ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
                rs->FrontCounterClockwise   = (desc->cull_mode == RHI_CULL_CCW) ? FALSE : TRUE;
                rs->DepthBias               = 0;
                rs->DepthBiasClamp          = 0.f;
                rs->SlopeScaledDepthBias    = 0.f;
                rs->DepthClipEnable         = !desc->disable_depth_clip;
                rs->MultisampleEnable       = FALSE;
                rs->AntialiasedLineEnable   = FALSE;
                rs->ForcedSampleCount       = 0;
                rs->ConservativeRaster      = desc->conservative_raster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
            }

            { // Depth Stencil
                auto *ds = &pipeline_desc.DepthStencilState;
                ds->DepthEnable    = desc->depth_enabled;
                if (desc->depth_enabled) {
                    ds->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                    ds->DepthFunc      = d3d12_comparison_func_from_rhi(desc->depth_compare_op);
                }
                ds->StencilEnable  = FALSE;
            }

            // Modern GPUs have fast raw load paths. We simply remove vertex buffer bindings.
            pipeline_desc.InputLayout           = { NULL, 0 };

            pipeline_desc.PrimitiveTopologyType = d3d12_primitive_topology_type_from_rhi(desc->topology);

            pipeline_desc.NumRenderTargets      = desc->num_color_attachments;

            for (u32 i = 0; i < desc->num_color_attachments; ++i) {
                pipeline_desc.RTVFormats[i] = dxgi_format_from_rhi(desc->color_attachment_formats[i]);
            }

            pipeline_desc.DSVFormat  = dxgi_format_from_rhi(desc->depth_format);

            pipeline_desc.SampleDesc = { 1, 0 };

            pipeline_desc.NodeMask   = NODE_MASK;

            // PSO Cache
            if (desc->cache && desc->cache_size > 0) {
                pipeline_desc.CachedPSO.pCachedBlob           = desc->cache;
                pipeline_desc.CachedPSO.CachedBlobSizeInBytes = desc->cache_size;
            }

            pipeline_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            hr = device->d3d12.device_10->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline->d3d12.state));
            if (FAILED(hr)) {
                log(LOG_ERROR, S("CreateGraphicsPipelineState failed: %S, %x"), string_from_hresult(hr), hr);
                return false;
            }
        } break;

        case RHI_PIPELINE_TYPE_COMPUTE: {
            Assert(!"Under construction.");
        } break;

        default: {
            Assert(!"Undefined pipeline type.");
        } break;
    }

    log(LOG_INFO, S("Initialized d3d12 pipeline."));
    return true;
}

void d3d12_pipeline_deinit(RHI_Pipeline *pipeline) {
    COM_SAFE_RELEASE(&pipeline->d3d12.state);
    log(LOG_INFO, S("Deinitialized d3d12 pipeline."));
}
