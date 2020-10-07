//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "utility.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"

//----------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

std::string ConvertUTF16ToUTF8(wchar_t *utf16) {
    auto size = WideCharToMultiByte(CP_ACP, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size, ' ');
    WideCharToMultiByte(CP_ACP, 0, utf16, -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

//----------------------------------------------------------------------------------------------------------------------

inline HRESULT CreateBuffer(ID3D12Device *device, D3D12_HEAP_TYPE heap_type, UINT64 size,
                            D3D12_RESOURCE_STATES resource_state, ID3D12Resource **buffer) {
    return device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heap_type), D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(size), resource_state,
                                           nullptr, IID_PPV_ARGS(buffer));
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CompileShader(const std::filesystem::path &file_path, const std::string &entrypoint,
                      const std::string &target, ID3DBlob **code) {
    return CompileShader(file_path, nullptr, entrypoint, target, code);
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CompileShader(const std::filesystem::path &file_path, const D3D_SHADER_MACRO *defines,
                      const std::string &entrypoint, const std::string &target, ID3DBlob **code) {
    UINT flags = 0;

#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> error;
    auto result = D3DCompileFromFile(file_path.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(),
                                     target.c_str(), flags, 0, code, &error);
    if (error) {
        std::cerr << static_cast<char *>(error->GetBufferPointer()) << std::endl;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *desc, ID3DBlob **blob) {
    ComPtr<ID3DBlob> error;
    auto result = D3D12SerializeRootSignature(desc, D3D_ROOT_SIGNATURE_VERSION_1, blob, &error);
    if (error) {
        std::cerr << static_cast<char *>(error->GetBufferPointer()) << std::endl;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CreateRootSignature(ID3D12Device *device, const D3D12_ROOT_SIGNATURE_DESC *desc,
                            ID3D12RootSignature **root_signature) {
    ComPtr<ID3DBlob> serialized_root_signature;
    auto result = SerializeRootSignature(desc, &serialized_root_signature);
    if (FAILED(result)) {
        return result;
    }

    return device->CreateRootSignature(0, serialized_root_signature->GetBufferPointer(),
                                       serialized_root_signature->GetBufferSize(), IID_PPV_ARGS(root_signature));
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CreateDefaultBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer) {
    return CreateBuffer(device, D3D12_HEAP_TYPE_DEFAULT, size, D3D12_RESOURCE_STATE_COPY_DEST, buffer);
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CreateUploadBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer) {
    return CreateBuffer(device, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ, buffer);
}

//----------------------------------------------------------------------------------------------------------------------

void UpdateBuffer(ID3D12Resource *buffer, void *data, UINT64 size) {
    void *contents;
    ThrowIfFailed(buffer->Map(0, nullptr, &contents));
    memcpy(contents, data, size);
    buffer->Unmap(0, nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CreateConstantBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer) {
    return CreateBuffer(device, D3D12_HEAP_TYPE_UPLOAD,
                        AlignPow2(sizeof(size), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
                        D3D12_RESOURCE_STATE_GENERIC_READ, buffer);
}

//----------------------------------------------------------------------------------------------------------------------

#pragma clang diagnostic pop