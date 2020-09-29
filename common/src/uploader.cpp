//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "uploader.h"

#include <d3dx12.h>
#include <dxgi1_6.h>
#include <array>

#include "utility.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"

//----------------------------------------------------------------------------------------------------------------------

constexpr std::array<D3D12_COMMAND_LIST_TYPE, 2> kCommandTypes = {D3D12_COMMAND_LIST_TYPE_COPY,
                                                                  D3D12_COMMAND_LIST_TYPE_DIRECT};

//----------------------------------------------------------------------------------------------------------------------

Uploader::Uploader(ID3D12Device4 *device)
        : _device(device) {
    InitCommandQueues();
    InitCommandAllocators();
    InitCommandLists();
    InitFence();
    InitEvent();
}

//----------------------------------------------------------------------------------------------------------------------

Uploader::~Uploader() {
    TermEvent();
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::RecordCopyData(ID3D12Resource *buffer, void *data, UINT64 size) {
    // Create an upload buffer.
    ComPtr<ID3D12Resource> upload_buffer;
    ThrowIfFailed(CreateUploadBuffer(_device, size, &upload_buffer));

    // Keep an upload buffer until a command list is completed.
    _upload_buffers.push_back(upload_buffer);

    // Define a subresource data.
    D3D12_SUBRESOURCE_DATA copy_desc = {};
    copy_desc.pData = data;
    copy_desc.RowPitch = size;
    copy_desc.SlicePitch = copy_desc.RowPitch;

    // Record commands.
    UpdateSubresources<1>(_command_lists[0].Get(), buffer, upload_buffer.Get(), 0, 0, 1, &copy_desc);
    _command_lists[1]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::Execute() {
    // Finish recording command lists.
    for (auto i = 0; i != 2; ++i) {
        _command_lists[i]->Close();
    }

    ID3D12CommandList* command_lists[] {_command_lists[0].Get(), _command_lists[1].Get()};

    // Execute copy commands.
    _command_queues[0]->ExecuteCommandLists(1, &command_lists[0]);

    // Make a dependency between a copy queue and a direct queue.
    _command_queues[0]->Signal(_fence.Get(), 1);
    _command_queues[1]->Wait(_fence.Get(), 1);

    // Execute transition commands.
    _command_queues[1]->ExecuteCommandLists(1, &command_lists[1]);
    _command_queues[1]->Signal(_fence.Get(), 2);

    // Wait until a command list is completed.
    if (_fence->GetCompletedValue() < 2) {
        _fence->SetEventOnCompletion(2, _event);
        WaitForSingleObject(_event, INFINITE);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::InitCommandQueues() {
    for (auto i = 0; i != 2; ++i) {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = kCommandTypes[i];
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_command_queues[i])));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::InitCommandAllocators() {
    for (auto i = 0; i != 2; ++i) {
        ThrowIfFailed(_device->CreateCommandAllocator(kCommandTypes[i], IID_PPV_ARGS(&_command_allocators[i])));
    }

}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::InitCommandLists() {
    for (auto i = 0; i != 2; ++i) {
        ThrowIfFailed(_device->CreateCommandList(0, kCommandTypes[i], _command_allocators[i].Get(), nullptr,
                                                 IID_PPV_ARGS(&_command_lists[i])));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::InitFence() {
    ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::InitEvent() {
    _event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!_event) {
        throw std::runtime_error("Fail to create an event.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Uploader::TermEvent() {
    CloseHandle(_event);
    _event = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma clang diagnostic pop