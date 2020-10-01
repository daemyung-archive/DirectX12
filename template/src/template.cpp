//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include <common/window.h>
#include <common/example.h>
#include <iostream>
#include <memory>

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount}};

//----------------------------------------------------------------------------------------------------------------------

class Template : public Example {
public:
    Template() : Example("Template", kDescriptorCount) {
    }

protected:
    void OnInit() override {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        handle = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            _device->CreateRenderTargetView(_swap_chain_buffers[i].Get(), nullptr, handle);
            handle.ptr += _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
        }
    }

    void OnTerm() override {
    }

    void OnResize(const Resolution& resolution) override {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        handle = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            _device->CreateRenderTargetView(_swap_chain_buffers[i].Get(), nullptr, handle);
            handle.ptr += _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
        }
    }

    void OnUpdate(UINT index) override {
    }

    void OnRender(UINT index) override {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = _swap_chain_buffers[index].Get();
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        _command_list->ResourceBarrier(1, &barrier);

        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(GetWidth(_resolution));
        viewport.Height = static_cast<float>(GetHeight(_resolution));
        viewport.MaxDepth = 1.0f;

        _command_list->RSSetViewports(1, &viewport);

        D3D12_RECT scissor = {};
        scissor.right = GetWidth(_resolution);
        scissor.bottom = GetHeight(_resolution);
        _command_list->RSSetScissorRects(1, &scissor);

        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        handle = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += _descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] * index;

        _command_list->ClearRenderTargetView(handle, DirectX::Colors::LightSteelBlue, 0, nullptr);
        _command_list->OMSetRenderTargets(1, &handle, true, nullptr);

        RecordDrawImGuiCommands();

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        _command_list->ResourceBarrier(1, &barrier);
    }
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<Template>();
        Window().MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        std::cerr << exception.what() << std::endl;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------