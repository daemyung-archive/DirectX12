//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "example.h"

#include "window.h"

//----------------------------------------------------------------------------------------------------------------------

Example::Example(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts) {
    InitFactory();
    InitAdapter();
    InitDevice();
    InitCommandQueue();
    InitCommandLists();
    InitCommandAllocators();
    InitFence();
    InitEvent();
    InitDescriptorHeaps(descriptor_counts);
    InitDescriptorHeapSizes();
}

//----------------------------------------------------------------------------------------------------------------------

Example::~Example() {
    TermEvent();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::BindToWindow(Window *window) {
    InitSwapChain(window);
    InitSwapChainBuffers();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Init() {
    // Build descriptors.
    OnBuildDescriptors();

    // Build command lists.
    for (auto i = 0; i != kSwapChainBufferCount; ++i) {
        ThrowIfFailed(_command_allocators[i]->Reset());
        ThrowIfFailed(_command_lists[i]->Reset(_command_allocators[i].Get(), nullptr));
        OnBuildCommands(i);
        ThrowIfFailed(_command_lists[i]->Close());
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Term() {
    WaitCommandQueueIdle();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Render() {
    // Retrieve the current index of a swap chain.
    auto index = _swap_chain->GetCurrentBackBufferIndex();
    assert(index < kSwapChainBufferCount);

    // Wait until a command list is completed.
    if (_fence->GetCompletedValue() < _fence_value_stamps[index]) {
        _fence->SetEventOnCompletion(_fence_value_stamps[index], _event);
        WaitForSingleObject(_event, INFINITE);
    }

    // Update uniforms.
    OnUpdateUniforms(_swap_chain->GetCurrentBackBufferIndex());

    // Execute a command list.
    std::vector<ID3D12CommandList *> command_lists = {_command_lists[index].Get()};
    _command_queue->ExecuteCommandLists(static_cast<UINT>(command_lists.size()), command_lists.data());
    ThrowIfFailed(_command_queue->Signal(_fence.Get(), ++_fence_value));
    _fence_value_stamps[index] = _fence_value;

    // Preset a swap chain image.
    ThrowIfFailed(_swap_chain->Present(0, 0));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::WaitCommandQueueIdle() {
    _command_queue->Signal(_fence.Get(), ++_fence_value);
    if (_fence->GetCompletedValue() < _fence_value) {
        _fence->SetEventOnCompletion(_fence_value, _event);
        WaitForSingleObject(_event, INFINITE);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitFactory() {
    UINT flags = 0;

#ifdef _DEBUG
    // Enable a debug layer if it is available.
    ComPtr<ID3D12Debug3> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&_factory)));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitAdapter() {
    ThrowIfFailed(_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                       IID_PPV_ARGS(&_adapter)));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitDevice() {
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_command_queue)));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitCommandAllocators() {
    for (auto i = 0; i != kSwapChainBufferCount; ++i) {
        ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(&_command_allocators[i])));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitCommandLists() {
    for (auto i = 0; i != kSwapChainBufferCount; ++i) {
        ThrowIfFailed(_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                                                  IID_PPV_ARGS(&_command_lists[i])));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitFence() {
    ThrowIfFailed(_device->CreateFence(_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

    for (auto i = 0; i != kSwapChainBufferCount; ++i) {
        _fence_value_stamps[i] = _fence_value;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitEvent() {
    _event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!_event) {
        throw std::runtime_error("Fail to create an event.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::TermEvent() {
    CloseHandle(_event);
    _event = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitDescriptorHeaps(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptorCount) {
    for (auto[type, count] : descriptorCount) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = type;
        desc.NumDescriptors = count;

        if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }

        ThrowIfFailed(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_descriptor_heaps[type])));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitDescriptorHeapSizes() {
    for (auto i = 0; i != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        _descriptor_heap_sizes[i] = _device->GetDescriptorHandleIncrementSize(
                static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitSwapChain(Window *window) {
    // Define a swap chain.
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = kSwapChainBufferCount;
    desc.Width = GetWidth(_resolution);
    desc.Height = GetHeight(_resolution);
    desc.Format = kSwapChainFormat;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc = {1, 0};

    ComPtr<IDXGISwapChain1> swap_chain;
    ThrowIfFailed(_factory->CreateSwapChainForHwnd(_command_queue.Get(), window->GetWindow(), &desc,
                                                   nullptr, nullptr, &swap_chain));

    // Get a swap chain interface.
    ThrowIfFailed(swap_chain->QueryInterface(IID_PPV_ARGS(&_swap_chain)));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::InitSwapChainBuffers() {
    for (auto i = 0; i != kSwapChainBufferCount; ++i) {
        ThrowIfFailed(_swap_chain->GetBuffer(i, IID_PPV_ARGS(&_swap_chain_buffers[i])));
    }
}

//----------------------------------------------------------------------------------------------------------------------
