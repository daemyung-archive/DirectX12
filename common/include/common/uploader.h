//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef UPLOADER_H_
#define UPLOADER_H_

#include <wrl.h>
#include <d3d12.h>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

class Uploader final {
public:
    //! Constructor.
    //! \param device A DirectX12 device.
    explicit Uploader(ID3D12Device4 *device);

    //! Destructor.
    ~Uploader();

    //! Record a copy data command.
    //! \param buffer A destination buffer.
    //! \param data The data will be copied to a destination buffer.
    //! \param size A size of the data.
    void RecordCopyData(ID3D12Resource *buffer, void *data, UINT64 size);

    //! Execute recorded copy data commands.
    void Execute();

private:
    //! Initialize command queues.
    void InitCommandQueues();

    //! Initialize command allocators.
    void InitCommandAllocators();

    //! Initialize command lists.
    void InitCommandLists();

    //! Initialize a fence.
    void InitFence();

    //! Initialize an event.
    void InitEvent();

    //! Terminate an event.
    void TermEvent();

private:
    ID3D12Device4 *_device = nullptr;
    ComPtr<ID3D12CommandQueue> _command_queues[2];
    ComPtr<ID3D12CommandAllocator> _command_allocators[2];
    ComPtr<ID3D12GraphicsCommandList4> _command_lists[2];
    ComPtr<ID3D12Fence> _fence;
    HANDLE _event = nullptr;
    std::vector<ComPtr<ID3D12Resource>> _upload_buffers;
};

//----------------------------------------------------------------------------------------------------------------------

#endif