// Copyright Seong Woo Lee. All Rights Reserved.

static void d3d12_log_message(D3D12_MESSAGE_SEVERITY severity, LPCSTR description) {
    String s = {};

    switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
            s = S("Corruption");
            break;

        case D3D12_MESSAGE_SEVERITY_ERROR:
            s = S("Error");
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            s = S("Warning");
            break;

        case D3D12_MESSAGE_SEVERITY_INFO:
            s = S("Info");
            break;

        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            s = S("Message");
            break;

        default:
            break;
    }

    log(S("[%S] %s"), s, description);
}

static void d3d12_flush_messages(ID3D12InfoQueue1 *info_queue) {
    // @Todo: Use temp allocator?
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
            break_debugger();
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            if (description) d3d12_log_message(severity, description);
            if (break_on_warning) break_debugger();
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
        log(S("HRESULT: %S, %x. ID3D12Device::CreateCommandQueue failed."), win32_string_from_hresult(hr), hr);
        return false;
    }

    queue->type = type;
    log(S("Initialized d3d12 command queue."));
    return true;
}

static void d3d12_queue_deinit(D3D12_Command_Queue *queue) {
    if (queue) {
        RHI_SAFE_RELEASE(&queue->queue_0);
    }
    log(S("Deinitialized d3d12 command queue."));
}

//
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
        log(S("HRESULT: %S, %x. ID3D12Device::CreateDescriptorHeap failed."), win32_string_from_hresult(hr), hr);
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

    log(S("Initialized d3d12 '%S' heap."), type_str);
    return true;
}

static void d3d12_descriptor_heap_deinit(D3D12_Descriptor_Heap *heap) {
    if (heap) {
        RHI_SAFE_RELEASE(&heap->heap_0);
        dealloc(heap->free_list);
    }
}

static D3D12_Descriptor d3d12_descriptor_alloc(D3D12_Descriptor_Heap *heap) {
    int index = -1;

    // @Todo: One option to make it faster is making it hierarchical.
    for (u32 i = 0; i < heap->free_list_node_count; ++i) {
        u64 bits = heap->free_list[i];
        u64 b = tzcnt64(bits);
        if (b < 64) {
            bits ^= (1ull << b);
            index = i * 64 + b;
            heap->free_list[i] = bits;
            break;
        }
    }

    // @Todo: grow?
    Assert(index != -1);

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


//
// Device
//
bool d3d12_device_init(RHI_Device *device, bool debug, bool break_on_warning) {
    HRESULT hr = S_OK;
    auto *d3d12 = &device->d3d12;

    IDXGIDebug       *dxgi_debug        = NULL;
    ID3D12Debug      *debug_interface   = NULL;
    ID3D12Debug5     *debug_interface_5 = NULL;
    ID3D12InfoQueue1 *info_queue_1      = NULL;

    d3d12->break_on_warning = break_on_warning;

    if (debug) {
        d3d12->dxgi_debug_dll_handle = LoadLibrary(L"dxgidebug.dll");
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

                RHI_SAFE_RELEASE(&debug_interface_5);
            } else {
                log(S("HRESULT: %x. Failed to query interface 'ID3D12Debug5'. 'Windows 10 Build 20348' is the minimum supported version."), hr);
                return false;
            }

            RHI_SAFE_RELEASE(&debug_interface);
        } else {
            log(S("HRESULT: %x. Failed to get 'ID3D12Debug' interface."), hr);
            return false;
        }
    }


    //
    // @Todo: DRED (https://microsoft.github.io/DirectX-Specs/d3d/DeviceRemovedExtendedData.html)
    //


    IDXGIFactory6 *dxgi_factory_6 = NULL;
    {
        UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
        hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgi_factory_6));
        if (FAILED(hr)) {
            log(S("HRESULT: %x. 'CreateDXGIFactory2' failed on 'IDXGIFactory6'. 'IDXGIFactory6' is supported from Windows 10, version 1803."), hr);
            return false;
        }
    }


    IDXGIAdapter1 *adapter_1 = NULL;
    {
        UINT adapter_index = 0;
        DXGI_GPU_PREFERENCE preference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        hr = dxgi_factory_6->EnumAdapterByGpuPreference(adapter_index, preference, IID_PPV_ARGS(&adapter_1));
        if (FAILED(hr)) {
            log(S("HRESULT: %x. 'IDXGIFactory6::EnumAdapterByGpuPreference()' failed."), hr);
            return false;
        }
    }


    ID3D12Device *device_0 = NULL;
    {
        D3D_FEATURE_LEVEL minimum_feature_level = D3D_FEATURE_LEVEL_12_0;
        hr = D3D12CreateDevice(adapter_1, minimum_feature_level, IID_PPV_ARGS(&device_0));
        if (FAILED(hr)) {
            log(S("HRESULT: %x. D3D12CreateDevice() failed."), hr);
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
                    log(S("ID3D12InfoQueue1::RegisterMessageCallback failed."));
                }
            } else {
                log(S("HRESULT: %x. ID3D12InfoQueue::QueryInterface(ID3D12InfoQueue1 *) failed."), hr);

                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,    break_on_warning);
            }

            RHI_SAFE_RELEASE(&info_queue_0);
        } else {
            log(S("HRESULT: %x. ID3D12Device::QueryInterface(ID3D12InfoQueue *) failed."), hr);
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
            log(S("The device doesn't support shader model 6.6."));
            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        device_0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
        if (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3) {
            log(S("Resource binding tier: %d"), options.ResourceBindingTier);
        } else {
            log(S("The device should have resource binding tier 3 or greater."));
            return false;
        }
    }


    {
        //
        // Check for enhanced barrier feature support.
        //
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
        hr = device_0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));

        if (FAILED(hr)) {
            log(S("HRESULT: %S, %x. CheckFeatureSupport failed."), win32_string_from_hresult(hr), hr);
            return false;
        }

        if (!options12.EnhancedBarriersSupported) {
            log(S("Enhanced barrier is not supported on this device."));
            return false;
        }
    }


    ID3D12Device12 *device_10 = NULL;
    if (FAILED(device_0->QueryInterface(IID_PPV_ARGS(&device_10)))) {
        log(S("QueryInterface() for ID3D12Device10 failed. Requires DirectX 12 Agility SDK 1.7 or later."));
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
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->rtv_heap,      D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         2048)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->dsv_heap,      D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         2048)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->resource_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048)) return false;
    if (!d3d12_descriptor_heap_init(d3d12, &d3d12->sampler_heap,  D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     2048)) return false;


    // Create command queues.
    for (RHI_Command_Type type = RHI_COMMAND_TYPE_GRAPHICS; type < RHI_COMMAND_TYPE_COUNT; ++type) {
        d3d12_queue_init(d3d12, &d3d12->queues[type], d3d12_translate_queue_type(type));
    }


    // Cleanup
    RHI_SAFE_RELEASE(&adapter_1);


    log(S("Initialized d3d12 device."));
    return true;
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

    RHI_SAFE_RELEASE(&d->dxgi_factory_6);
    RHI_SAFE_RELEASE(&d->device_10);
    RHI_SAFE_RELEASE(&d->device_0);

    if (d->info_queue_1 && d->callback_cookie != 0) {
        d->info_queue_1->UnregisterMessageCallback(d->callback_cookie);
        d->callback_cookie = 0;
    }

    if (d->dxgi_debug) {
        // @Todo: Since info_queue_1 isn't released, there's a log saying there's a leak.
        d->dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        RHI_SAFE_RELEASE(&d->dxgi_debug);
    }

    if (d->info_queue_1) {
        d3d12_flush_messages(d->info_queue_1);
        RHI_SAFE_RELEASE(&d->info_queue_1);
    }



    if (d->dxgi_debug_dll_handle) {
        FreeLibrary(d->dxgi_debug_dll_handle);
        d->dxgi_debug_dll_handle = {};
    }


    log(S("Deinitialized d3d12 device"));
}

//
// Command List
//
bool d3d12_command_list_init(D3D12_Device *device, D3D12_Command_List *list, RHI_Command_Type type) {
    auto native_type = d3d12_translate_queue_type(type);
    list->type = native_type;

    HRESULT hr = device->device_10->CreateCommandAllocator(native_type, IID_PPV_ARGS(&list->allocator));
    if (FAILED(hr)) {
        log(S("HRESULT: %x. CreateCommandAllocator failed."), hr);
        return false;
    }

    hr = device->device_10->CreateCommandList1(NODE_MASK, native_type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&list->list_0));
    if (FAILED(hr)) {
        log(S("HRESULT: %x. CreateCommandList1 failed."), hr);
        return false;
    }

    hr = list->list_0->QueryInterface(IID_PPV_ARGS(&list->list_7));
    if (FAILED(hr)) {
        log(S("HRESULT: %x. Failed to query interface for ID3D12GraphicsCommandList7. Requires the DirectX 12 Agility SDK 1.7 or later."), hr);
        return false;
    }

    log(S("Initialized d3d12 command list."));
    return true;
}

void d3d12_command_list_deinit(D3D12_Command_List *list) {
    if (list) {
        RHI_SAFE_RELEASE(&list->list_0);
        RHI_SAFE_RELEASE(&list->list_7);
        RHI_SAFE_RELEASE(&list->allocator);
    }
}

void d3d12_command_list_begin(D3D12_Command_List *list) {
    // Although an ID3D12GraphicsCommandList can be reset immediately after
    // execution (provided it is associated with a different allocator, or the
    // current allocator is no longer in use by the GPU), our abstraction keeps a
    // 1:1 relationship between a command list and its allocator.
    //
    // Therefore, the caller must ensure the GPU has finished executing commands
    // recorded with this allocator before calling d3d12_command_list_begin().
    //
    if (!list->is_recording) {
        list->allocator->Reset();
        list->list_7->Reset(list->allocator, NULL);
        list->is_recording = true;
    } else {
        log(S("Command list wasn't closed."));
    }
}

void d3d12_command_list_end(D3D12_Command_List *list) {
    if (list->is_recording) {
        list->list_7->Close();
        list->is_recording = false;
    } else {
        log(S("Command list was already closed."));
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


//
// Render Pass
//
void d3d12_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Pass *pass) {
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

}


//
// Texture
//
static DXGI_FORMAT d3d12_texture_format(RHI_Texture_Format format) {
    switch (format) {
        case RHI_TEXTURE_FORMAT_UNKNOWN:
            return DXGI_FORMAT_UNKNOWN;

        case RHI_TEXTURE_FORMAT_R8_UNORM:
            return DXGI_FORMAT_R8_UNORM;

        case RHI_TEXTURE_FORMAT_RG8_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;

        case RHI_TEXTURE_FORMAT_RGBA8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case RHI_TEXTURE_FORMAT_BGRA8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case RHI_TEXTURE_FORMAT_RGBA8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case RHI_TEXTURE_FORMAT_BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case RHI_TEXTURE_FORMAT_R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        case RHI_TEXTURE_FORMAT_RG16_UNORM:
            return DXGI_FORMAT_R16G16_UNORM;

        case RHI_TEXTURE_FORMAT_RGBA16_UNORM:
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case RHI_TEXTURE_FORMAT_R16F:
            return DXGI_FORMAT_R16_FLOAT;

        case RHI_TEXTURE_FORMAT_RG16F:
            return DXGI_FORMAT_R16G16_FLOAT;

        case RHI_TEXTURE_FORMAT_RGBA16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case RHI_TEXTURE_FORMAT_R32F:
            return DXGI_FORMAT_R32_FLOAT;

        case RHI_TEXTURE_FORMAT_RG32F:
            return DXGI_FORMAT_R32G32_FLOAT;

        case RHI_TEXTURE_FORMAT_RGBA32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case RHI_TEXTURE_FORMAT_D32F:
            return DXGI_FORMAT_D32_FLOAT;

        case RHI_TEXTURE_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;

        case RHI_TEXTURE_FORMAT_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case RHI_TEXTURE_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;

        case RHI_TEXTURE_FORMAT_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case RHI_TEXTURE_FORMAT_BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;

        case RHI_TEXTURE_FORMAT_BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;

        case RHI_TEXTURE_FORMAT_BC6H_UFLOAT:
            return DXGI_FORMAT_BC6H_UF16;

        case RHI_TEXTURE_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;

        case RHI_TEXTURE_FORMAT_BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            Assert(!"Unknown texture format.");
            return DXGI_FORMAT_UNKNOWN;
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

bool d3d12_texture_create(RHI_Device *device, RHI_Texture *texture, RHI_Texture_Desc *desc, RHI_Heap *heap) {
    D3D12_RESOURCE_DESC resource_desc = {};
    {
        resource_desc.Dimension         = d3d12_resource_dimension_from_rhi_texture_type(desc->type);
        resource_desc.Alignment         = 0; // effectively 64KB.
        resource_desc.Width             = desc->width;
        resource_desc.Height            = desc->height;
        resource_desc.DepthOrArraySize  = desc->depth;
        resource_desc.MipLevels         = desc->mip_levels;
        resource_desc.Format            = d3d12_texture_format(desc->format);
        resource_desc.SampleDesc        = { 1, 0 };
        resource_desc.Layout            = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resource_desc.Flags             = d3d12_resource_flags_from_texture_usage(desc->usage);
    }

    if (heap) {
        Assert(!"User heap not supported at the moment.");
    } else {
        D3D12_HEAP_PROPERTIES heap_prop = {};
        heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = device->d3d12.device_10->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, 
                                                                      &resource_desc, D3D12_RESOURCE_STATE_COMMON, 
                                                                      NULL, IID_PPV_ARGS(&texture->d3d12.resource));
        if (FAILED(hr)) {
            log(S("HRESULT: %S, %x. CreateCommittedResource failed."), win32_string_from_hresult(hr), hr);
            return false;
        }
    }

    log(S("Created d3d12 texture."));
    return true;
}

void d3d12_texture_destroy(RHI_Texture *texture) {
    // Make sure the texture isn't in flight!
    RHI_SAFE_RELEASE(&texture->d3d12.resource);
    log(S("Destroyed d3d12 texture."));
}

static D3D12_SHADER_RESOURCE_VIEW_DESC d3d12_srv_desc(RHI_Texture_View_Desc *desc) {
    D3D12_SHADER_RESOURCE_VIEW_DESC result = {};
    result.Format = d3d12_texture_format(desc->format);
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
            t->FirstArraySlice      = desc->base_array_slice;
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

static D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12_uav_desc(RHI_Texture_View_Desc *desc) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC result = {};
    result.Format = d3d12_texture_format(desc->format);

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
            t->FirstArraySlice      = desc->base_array_slice;
            t->ArraySize            = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_3D: {
            // @Todo: Not sure of WSlice and WSize.
            auto *t = &result.Texture3D;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE3D;
            t->MipSlice             = desc->base_mip_level;
            t->FirstWSlice          = desc->base_array_slice;
            t->WSize                = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_CUBE: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_slice;
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
    result.Format = d3d12_texture_format(desc->format);

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
            t->FirstArraySlice      = desc->base_array_slice;
            t->ArraySize            = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_3D: {
            // @Todo: Not sure of WSlice and WSize.
            auto *t = &result.Texture3D;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE3D;
            t->MipSlice             = desc->base_mip_level;
            t->FirstWSlice          = desc->base_array_slice;
            t->WSize                = desc->depth;
        } break;

        case RHI_TEXTURE_TYPE_CUBE: {
            auto *t = &result.Texture2DArray;
            result.ViewDimension    = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            t->MipSlice             = desc->base_mip_level;
            t->FirstArraySlice      = desc->base_array_slice;
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
    result.Format = d3d12_texture_format(desc->format);

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
            t->FirstArraySlice      = desc->base_array_slice;
            t->ArraySize            = desc->depth;
        } break;

        default: {
            Assert(!"Invalid texture dimension.");
        } break;
    }

    return result;
}

void d3d12_texture_view_create(RHI_Device *device, RHI_Texture_View *view, RHI_Texture *texture, RHI_Texture_View_Desc *desc) {
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
}

void d3d12_texture_view_destroy(RHI_Texture_View *view) {
    d3d12_descriptor_dealloc(&view->d3d12);
    memset(view, 0, sizeof(*view));
}


//
// Surface
//
bool d3d12_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc *desc) {
    HWND hwnd = (HWND)desc->native_window_handle;

    //
    // @Note: Thank you Martins. 
    // (https://gist.github.com/mmozeiko/5e727f845db182d468a34d524508ad5f#file-win32_d3d11-c-L184-L185)
    //
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    {
        swap_chain_desc.Width  = desc->width;
        swap_chain_desc.Height = desc->height;

        swap_chain_desc.Format = RHI_D3D12_SURFACE_FORMAT; // @Temporary

        swap_chain_desc.Stereo = RHI_D3D12_SWAP_CHAIN_STEREO;

        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swap_chain_desc.BufferCount = desc->num_back_buffers,
        swap_chain_desc.Scaling     = DXGI_SCALING_STRETCH;

        // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD.
        // For Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL.
        // For Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD.
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        // FLIP presentation model does not allow MSAA framebuffer.
        // If you want MSAA then you'll need to render offscreen and manually
        // resolve to non-MSAA framebuffer.
        swap_chain_desc.SampleDesc = { 1, 0 },

        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        // @Todo: Allow tearing?
        swap_chain_desc.Flags = {}; 
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc = NULL;
    IDXGIOutput *monitor = NULL;

    IDXGISwapChain1 *swap_chain_1 = NULL;

    HRESULT hr = device->d3d12.dxgi_factory_6->CreateSwapChainForHwnd(device->d3d12.queues[RHI_COMMAND_TYPE_GRAPHICS].queue_0,
                                                                      hwnd, &swap_chain_desc, fullscreen_desc,  
                                                                      monitor, &swap_chain_1);

    if (FAILED(hr)) {
        log(S("HRESULT: %S, %x. IDXGIFactory6::CreateSwapChainForHwnd failed."), win32_string_from_hresult(hr), hr);
        return false;
    }

    hr = swap_chain_1->QueryInterface(IID_PPV_ARGS(&surface->d3d12.swap_chain_4));
    if (FAILED(hr)) {
        log(S("HRESULT: %S, %x. QueryInterface(IDXGISwapChain4 *) failed."), win32_string_from_hresult(hr), hr);
        return false;
    }

    // Disable Alt + Enter changing monitor resolution to match window size.
    device->d3d12.dxgi_factory_6->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);


    // Get resources from the swap chain and create render target views.
    for (u32 i = 0; i < desc->num_back_buffers; ++i) {
        auto *tex = &surface->textures[i];

        hr = surface->d3d12.swap_chain_4->GetBuffer(i, IID_PPV_ARGS(&tex->d3d12.resource));
        if (FAILED(hr)) {
            log(S("HRESULT: %S, %x. IDXGISwapChain1::GetBuffer failed."), win32_string_from_hresult(hr), hr);
            return false;
        }

        tex->kind = RHI_KIND_D3D12;
        tex->desc.type       = RHI_TEXTURE_TYPE_2D;
        tex->desc.format     = RHI_SURFACE_FORMAT;
        tex->desc.usage      = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT;
        tex->desc.width      = desc->width;
        tex->desc.height     = desc->height;
        tex->desc.mip_levels = 1;
        tex->desc.depth      = 1;
    }

    surface->current_frame_index = surface->d3d12.swap_chain_4->GetCurrentBackBufferIndex();

    log(S("Initialized d3d12 surface."));
    return true;
}

void d3d12_surface_present(RHI_Surface *surface) {
    // @Temporary
    //surface->swap_chain_4->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    surface->d3d12.swap_chain_4->Present(1, 0);
    surface->current_frame_index = surface->d3d12.swap_chain_4->GetCurrentBackBufferIndex();
}


//
// Fence
//
bool d3d12_fence_create(RHI_Device *device, RHI_Semaphore *fence) {
    UINT64 initial_value = 0;
    D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

    HRESULT hr = device->d3d12.device_10->CreateFence(initial_value, flags, IID_PPV_ARGS(&fence->d3d12.fence_0));

    if (FAILED(hr)) {
        log(S("HRESULT: %S, %x. CreateFence failed."), win32_string_from_hresult(hr), hr);
        return false;
    }

    fence->d3d12.event = CreateEvent(NULL, FALSE, FALSE, NULL);

    log(S("Created d3d12 fence."));
    return true;
}

void d3d12_fence_destroy(RHI_Semaphore *fence) {
    RHI_SAFE_RELEASE(&fence->d3d12.fence_0);
    if (fence->d3d12.event) {
        CloseHandle(fence->d3d12.event);
        fence->d3d12.event = {};
    }

    log(S("Destroyed d3d12 fence."));
}

void d3d12_fence_wait(RHI_Semaphore *fence, u64 value, u32 timeout) {
    if (fence->d3d12.fence_0->GetCompletedValue() < value) {
        fence->d3d12.fence_0->SetEventOnCompletion(value, fence->d3d12.event);
        WaitForSingleObject(fence->d3d12.event, (timeout == RHI_INFINITE) ? INFINITE : (DWORD)timeout);
    }
}


//
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

    bool all_mips   = (  mip == RHI_ALL_MIP_LEVELS  ) ? true : false;
    bool all_slices = (slice == RHI_ALL_ARRAY_SLICES) ? true : false;

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

void d3d12_queue_signal(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    auto *queue = &device->d3d12.queues[queue_type];
    queue->queue_0->Signal(semaphore->d3d12.fence_0, value);
}

void d3d12_queue_wait(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    auto *queue = &device->d3d12.queues[queue_type];
    queue->queue_0->Wait(semaphore->d3d12.fence_0, value);
}
