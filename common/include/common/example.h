//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <string>
#include <array>
#include <unordered_map>

#include "d3dx12.h"
#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

class Window;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto kSwapChainBufferCount = 2;
constexpr auto kSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr auto kFHDResolution = Resolution(1280, 720);

//----------------------------------------------------------------------------------------------------------------------

template<typename T>
using FrameResource = std::array<ComPtr<T>, kSwapChainBufferCount>;

//----------------------------------------------------------------------------------------------------------------------

class Example {
public:
    /// Constructor.
    /// \param descriptor_counts The number of descriptor in each heap.
    explicit Example(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptor_counts);

    /// Destructor.
    virtual ~Example();

    /// Bind DirectX to a window.
    /// \param window A window is bounded by DirectX.
    void BindToWindow(Window *window);

    /// Initialize.
    void Init();

    /// Terminate.
    void Term();

    /// Render.
    void Render();

    /// Retrieve a resolution.
    /// \return A resolution.
    [[nodiscard]]
    inline auto GetResolution() const {
        return _resolution;
    }

protected:
    /// Wait until a command queue is idle.
    void WaitCommandQueueIdle();

    /// Handle build descriptors event.
    virtual void OnBuildDescriptors() = 0;

    /// Handle build commands event.
    /// \param index The current index of swap chain image.
    virtual void OnBuildCommands(UINT index) = 0;

    /// Handle update uniforms event.
    /// \param index The current index of swap chain image.
    virtual void OnUpdateUniforms(UINT index) = 0;

protected:
    /// Initialize a factory.
    void InitFactory();

    /// Initialize an adapter.
    void InitAdapter();

    /// Initialize a device.
    void InitDevice();

    /// Initialize a command queue.
    void InitCommandQueue();

    /// Initialize a command allocator.
    void InitCommandAllocators();

    /// Initialize a command list.
    void InitCommandLists();

    /// Initialize a fence.
    void InitFence();

    /// Initialize an event.
    void InitEvent();

    /// Terminate an event.
    void TermEvent();

    /// Initialize descriptor heap.
    /// \param descriptorCount The number of descriptor in each heap.
    void InitDescriptorHeaps(const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> &descriptorCount);

    /// Initialize descriptor heap size.
    void InitDescriptorHeapSizes();

    /// Initialize a swap chain.
    /// \param window A window.
    void InitSwapChain(Window *window);

    /// Initialize a swap chain buffer.
    void InitSwapChainBuffers();

protected:
    Resolution _resolution = kFHDResolution;
    ComPtr<IDXGIFactory7> _factory;
    ComPtr<IDXGIAdapter4> _adapter;
    ComPtr<ID3D12Device4> _device;
    ComPtr<ID3D12CommandQueue> _command_queue;
    FrameResource<ID3D12CommandAllocator> _command_allocators;
    FrameResource<ID3D12GraphicsCommandList4> _command_lists;
    ComPtr<ID3D12Fence> _fence;
    UINT64 _fence_value = 0;
    UINT64 _fence_value_stamps[kSwapChainBufferCount] = {};
    HANDLE _event = nullptr;
    ComPtr<ID3D12DescriptorHeap> _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    UINT _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
    ComPtr<IDXGISwapChain3> _swap_chain;
    FrameResource<ID3D12Resource> _swap_chain_buffers;
};

//----------------------------------------------------------------------------------------------------------------------

#endif