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

void d3d12_message_callback(D3D12_MESSAGE_CATEGORY category, 
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

bool d3d12_device_init(RHI_Device *device, bool debug, bool break_on_warning) {
    HRESULT hr = S_OK;

    IDXGIDebug *dxgi_debug          = NULL;
    ID3D12Debug *debug_interface    = NULL;
    ID3D12Debug5 *debug_interface_5 = NULL;
    ID3D12InfoQueue1 *info_queue_1  = NULL;
    HMODULE dxgi_debug_dll_handle   = {};

    device->d3d12.break_on_warning = break_on_warning;

    if (debug) {
        dxgi_debug_dll_handle = LoadLibrary(L"dxgidebug.dll");
        if (dxgi_debug_dll_handle) {
            typedef HRESULT(WINAPI *DXGI_Get_Debug_Interface)(REFIID, void **);
            DXGI_Get_Debug_Interface dxgi_get_debug_interface = (DXGI_Get_Debug_Interface)(void*)(GetProcAddress(dxgi_debug_dll_handle, "DXGIGetDebugInterface"));
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

                debug_interface_5->Release();
                debug_interface_5 = NULL;
            } else {
                log(S("HRESULT: %x. Failed to query interface 'ID3D12Debug5'. 'Windows 10 Build 20348' is the minimum supported version."), hr);
                return false;
            }

            debug_interface->Release();
            debug_interface = NULL;
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
        } else {
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
            log(S("HRESULT: %x. ''D3D12CreateDevice()'' failed."), hr);
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
                info_queue_1->RegisterMessageCallback(d3d12_message_callback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, device, &device->d3d12.callback_cookie);
                info_queue_1->SetMuteDebugOutput(true);
                d3d12_flush_messages(info_queue_1);

                if (device->d3d12.callback_cookie == 0) {
                    log(S("ID3D12InfoQueue1::RegisterMessageCallback failed."));
                }
            } else {
                log(S("HRESULT: %x. ID3D12InfoQueue::QueryInterface(ID3D12InfoQueue1 *) failed."), hr);

                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      true);
                info_queue_0->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,    break_on_warning);
            }

            info_queue_0->Release();
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
        if (options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3) {
            log(S("The device should have resource binding tier 3 or greater."));
            return false;
        }
    }


    ID3D12Device12 *device_10 = NULL;
    if (FAILED(device_0->QueryInterface(IID_PPV_ARGS(&device_10)))) {
        log(S("QueryInterface() for ID3D12Device10 failed. Requires DirectX 12 Agility SDK 1.7 or later."));
        return false;
    }


    // Set fields.
    device->d3d12.dxgi_factory_6        = dxgi_factory_6;
    device->d3d12.device_0              = device_0;
    device->d3d12.device_10             = device_10;
    device->d3d12.info_queue_1          = info_queue_1;
    device->d3d12.dxgi_debug            = dxgi_debug;
    device->d3d12.dxgi_debug_dll_handle = dxgi_debug_dll_handle;


    // Cleanup
    if (adapter_1) adapter_1->Release();


    log(S("Successfully initialized D3D12_Device."));
    return true;
}

void d3d12_device_deinit(RHI_Device *device) {
    Assert(device->kind == RHI_KIND_D3D12);

    D3D12_Device *d = &device->d3d12;

    if (d->dxgi_factory_6) {
        d->dxgi_factory_6->Release();
        d->dxgi_factory_6 = NULL;
    }

    if (d->device_10) {
        d->device_10->Release();
        d->device_10 = NULL;
    }

    if (d->device_0) {
        d->device_0->Release();
        d->device_0 = NULL;
    }


    if (d->info_queue_1 && d->callback_cookie != 0) {
        d->info_queue_1->UnregisterMessageCallback(d->callback_cookie);
        d->callback_cookie = 0;
    }

    if (d->dxgi_debug) {
        // @Todo: Since info_queue_1 isn't released, there's a log saying there's a leak.
        d->dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        d->dxgi_debug->Release();
        d->dxgi_debug = NULL;
    }

    if (d->info_queue_1) {
        d3d12_flush_messages(d->info_queue_1);
        d->info_queue_1->Release();
        d->info_queue_1 = NULL;
    }



    if (d->dxgi_debug_dll_handle) {
        FreeLibrary(d->dxgi_debug_dll_handle);
        d->dxgi_debug_dll_handle = {};
    }


    log(S("Successfully deinitialized D3D12_Device."));
}
