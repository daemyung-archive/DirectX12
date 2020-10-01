//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "example.h"

#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "window.h"

//----------------------------------------------------------------------------------------------------------------------

Example::Example(const std::string &title,
                 const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts) :
        _title(title) {
    InitFactory();
    InitAdapter();
    InitDevice();
    InitCommandQueue();
    InitCommandList();
    InitCommandAllocators();
    InitFence();
    InitEvent();
    InitDescriptorHeaps(descriptor_counts);
    InitDescriptorHeapSizes();
}

//----------------------------------------------------------------------------------------------------------------------

Example::~Example() {
    TermImGui();
    TermEvent();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::BindToWindow(Window *window) {
    InitSwapChain(window);
    InitSwapChainBuffers();
    InitImGui(window);
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Init() {
    // Initialize by an example.
    OnInit();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Term() {
    WaitCommandQueueIdle();

    // Terminate by an example.
    OnTerm();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Resize(const Resolution &resolution) {
    // Resize swap chain buffers.
    WaitCommandQueueIdle();
    _swap_chain_buffers.fill(nullptr);
    _swap_chain->ResizeBuffers(kSwapChainBufferCount, GetWidth(resolution), GetHeight(resolution), kSwapChainFormat,
                               0);
    InitSwapChainBuffers();

    // Resize by an example.
    OnResize(resolution);
    _resolution = resolution;
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Update() {
    // Retrieve the current index of a swap chain.
    auto index = _swap_chain->GetCurrentBackBufferIndex();
    assert(index < kSwapChainBufferCount);

    // Wait until a command list is completed.
    if (_fence->GetCompletedValue() < _fence_value_stamps[index]) {
        _fence->SetEventOnCompletion(_fence_value_stamps[index], _event);
        WaitForSingleObject(_event, INFINITE);
    }

    // Update by an example.
    BeginImGuiPass();
    OnUpdate(index);
    EndImGuiPass();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::Render() {
    // Retrieve the current index of a swap chain.
    auto index = _swap_chain->GetCurrentBackBufferIndex();
    assert(index < kSwapChainBufferCount);

    // Render by an example.
    ThrowIfFailed(_command_allocators[index]->Reset());
    ThrowIfFailed(_command_list->Reset(_command_allocators[index].Get(), nullptr));
    OnRender(index);
    ThrowIfFailed(_command_list->Close());

    // Execute a command list.
    std::vector<ID3D12CommandList *> command_lists = {_command_list.Get()};
    _command_queue->ExecuteCommandLists(static_cast<UINT>(command_lists.size()), command_lists.data());
    ThrowIfFailed(_command_queue->Signal(_fence.Get(), ++_fence_value));
    _fence_value_stamps[index] = _fence_value;

    // Preset a swap chain image.
    ThrowIfFailed(_swap_chain->Present(0, 0));
}

//----------------------------------------------------------------------------------------------------------------------

void Example::RecordDrawImGuiCommands() {
    std::vector<ID3D12DescriptorHeap *> descriptor_heaps = {
            _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get()};
    _command_list->SetDescriptorHeaps(static_cast<UINT>(descriptor_heaps.size()), descriptor_heaps.data());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _command_list.Get());
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
    ThrowIfFailed(_adapter->GetDesc3(&_adapter_desc));
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

void Example::InitCommandList() {
    ThrowIfFailed(_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                                              IID_PPV_ARGS(&_command_list)));
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

void Example::InitDescriptorHeaps(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts) {
    for (auto[type, count] : descriptor_counts) {
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

void Example::InitImGui(Window *window) {
    // Initialize ImGUI.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Use the classic theme.
    ImGui::StyleColorsClassic();

    // Initialize ImGUI for Windows.
    ImGui_ImplWin32_Init(window->GetWindow());

    // Retrieve resource which are need to set up ImGUI.
    auto descriptor_heap = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get();
    auto offset = descriptor_heap->GetDesc().NumDescriptors - kImGuiFontBufferCount;
    auto size = _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    cpu_handle.Offset(offset, size);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(descriptor_heap->GetGPUDescriptorHandleForHeapStart());
    cpu_handle.Offset(offset, size);

    // Initialize ImGUI for DirectX12.
    ImGui_ImplDX12_Init(_device.Get(), kSwapChainBufferCount, kSwapChainFormat, descriptor_heap,
                        cpu_handle, gpu_handle);
}

//----------------------------------------------------------------------------------------------------------------------

void Example::TermImGui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//----------------------------------------------------------------------------------------------------------------------

void Example::BeginImGuiPass() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("DirectX12", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(_title.c_str());
    ImGui::TextUnformatted(ConvertUTF16ToUTF8(_adapter_desc.Description).c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void Example::EndImGuiPass() {
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::EndFrame();
    ImGui::Render();
}

//----------------------------------------------------------------------------------------------------------------------
