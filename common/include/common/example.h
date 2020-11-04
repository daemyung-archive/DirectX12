//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include <imgui.h>
#include <d3dx12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <string>
#include <array>
#include <unordered_map>

#include "utility.h"
#include "window.h"
#include "timer.h"
#include "camera.h"
#include "compiler.h"

//----------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto kSwapChainBufferCount = 2;
constexpr auto kSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr auto kImGuiFontBufferCount = 1;

//----------------------------------------------------------------------------------------------------------------------

template<typename T>
using FrameResource = std::array<ComPtr<T>, kSwapChainBufferCount>;

//----------------------------------------------------------------------------------------------------------------------

class Example {
public:
    //! Constructor.
    //! \param title The example title.
    //! \param descriptor_counts The number of descriptor in each heap.
    Example(const std::string &title,
            const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts);

    //! Destructor.
    virtual ~Example();

    //! Bind DirectX to a window.
    //! \param window A window is bounded by DirectX.
    void BindToWindow(Window *window);

    //! Initialize.
    void Init();

    //! Terminate.
    void Term();

    //! Resize.
    //! \param resolution A resolution.
    void Resize(const Resolution &resolution);

    //! Update.
    void Update();

    //! Render.
    void Render();

    //! Handle mouse button down event.
    //! \param button The button that is pressing.
    //! \param position A position of the mouse cursor.
    void OnMouseButtonDown(MouseButton button, const POINT &position);

    //! Handle mouse button up event.
    //! \param button The button that is releasing.
    //! \param position A position of the mouse cursor.
    void OnMouseButtonUp(MouseButton button, const POINT &position);

    //! Handle mouse move event.
    //! \param button The button that is pressing.
    //! \param position A position of the mouse cursor.
    void OnMouseMove(MouseButton button, const POINT &position);

    //! Handle mouse wheel event.
    //! \param delta The rotated distance by wheel.
    void OnMouseWheel(float delta);

protected:
    //! Record draw commands for ImGui.
    //! \param command_list A command list which can record commands.
    void RecordDrawImGuiCommands(ID3D12GraphicsCommandList* command_list);

    //! Wait until a command queue is idle.
    void WaitCommandQueueIdle();

    //! Handle initialize event.
    virtual void OnInit() = 0;

    //! Handle terminate event.
    virtual void OnTerm() = 0;

    //! Handle resize event.
    //! \param resolution A resolution.
    virtual void OnResize(const Resolution &resolution) = 0;

    //! Handle update event.
    //! \param index The current index of swap chain image.
    virtual void OnUpdate(UINT index) = 0;

    //! Handle render event.
    //! \param index The current index of swap chain image.
    virtual void OnRender(UINT index) = 0;

protected:
    //! Initialize a factory.
    void InitFactory();

    //! Initialize an adapter.
    void InitAdapter();

    //! Initialize a device.
    void InitDevice();

    //! Initialize a command queue.
    void InitCommandQueue();

    //! Initialize command allocators.
    void InitCommandAllocators();

    //! Initialize a command list.
    void InitCommandList();

    //! Initialize a fence.
    void InitFence();

    //! Initialize an event.
    void InitEvent();

    //! Terminate an event.
    void TermEvent();

    //! Initialize descriptor heap.
    //! \param descriptor_counts The number of descriptor in each heap.
    void InitDescriptorHeaps(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts);

    //! Initialize descriptor heap size.
    void InitDescriptorHeapSizes();

    //! Initialize a swap chain.
    //! \param window A window.
    void InitSwapChain(Window *window);

    //! Initialize a swap chain buffers.
    void InitSwapChainBuffers();

    //! Initialize a swap chain views.
    void InitSwapChainViews();

    //! Initialize ImGui.
    //! \param window A window.
    void InitImGui(Window *window);

    //! Terminate ImGui.
    void TermImGui();

    //! Begin ImGui pass.
    void BeginImGuiPass();

    //! End ImGui pass.
    void EndImGuiPass();

protected:
    std::string _title;
    Timer _timer;
    UINT _cps = 0;
    UINT _fps = 0;
    Duration _fps_time = Duration::zero();
    Compiler _compiler;
    Camera _camera;
    POINT _mouse_position = {0, 0};
    ComPtr<IDXGIFactory7> _factory;
    ComPtr<IDXGIAdapter4> _adapter;
    DXGI_ADAPTER_DESC3 _adapter_desc;
    ComPtr<ID3D12Device5> _device;
    ComPtr<ID3D12CommandQueue> _command_queue;
    FrameResource<ID3D12CommandAllocator> _command_allocators;
    ComPtr<ID3D12GraphicsCommandList4> _command_list;
    ComPtr<ID3D12Fence> _fence;
    UINT64 _fence_value = 0;
    UINT64 _fence_value_stamps[kSwapChainBufferCount] = {};
    HANDLE _event = nullptr;
    ComPtr<ID3D12DescriptorHeap> _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    UINT _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
    ComPtr<IDXGISwapChain3> _swap_chain;
    FrameResource<ID3D12Resource> _swap_chain_buffers;
    D3D12_CPU_DESCRIPTOR_HANDLE _swap_chain_views[kSwapChainBufferCount] = {};
};

//----------------------------------------------------------------------------------------------------------------------

#endif