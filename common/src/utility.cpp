//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "utility.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <iostream>
#include <fstream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

//----------------------------------------------------------------------------------------------------------------------

using namespace DirectX;
using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

inline HRESULT CreateBuffer(ID3D12Device *device, D3D12_HEAP_TYPE heap_type, UINT64 size,
                            D3D12_RESOURCE_STATES resource_state, ID3D12Resource **buffer) {
    return device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heap_type), D3D12_HEAP_FLAG_NONE,
                                           &CD3DX12_RESOURCE_DESC::Buffer(size), resource_state,
                                           nullptr, IID_PPV_ARGS(buffer));
}

//----------------------------------------------------------------------------------------------------------------------

std::string ConvertUTF16ToUTF8(wchar_t *utf16) {
    auto size = WideCharToMultiByte(CP_ACP, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size, ' ');
    WideCharToMultiByte(CP_ACP, 0, utf16, -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<BYTE> ReadFile(const std::filesystem::path &path) {
    std::basic_ifstream<BYTE> fin(path, std::ios::in | std::ios::binary);
    if (!fin.is_open()) {
        throw std::runtime_error(fmt::format("Fail to open {}.", path.string()));
    }
    return std::vector<BYTE>(std::istreambuf_iterator<BYTE>(fin), std::istreambuf_iterator<BYTE>());
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
                        AlignPow2(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
                        D3D12_RESOURCE_STATE_GENERIC_READ, buffer);
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT CreateDefaultTexture2D(ID3D12Device *device, UINT64 width, UINT height, UINT16 mip_levels,
                               DXGI_FORMAT format, ID3D12Resource **buffer) {
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = mip_levels;
    desc.Format = format;
    desc.SampleDesc = {1, 0};

    return device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                           &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(buffer));
}

//----------------------------------------------------------------------------------------------------------------------

DirectX::XMFLOAT3X4 XMMatrixInverseTranspose(const DirectX::XMFLOAT4X4 &float4x4) {
    XMMATRIX matrix = XMMatrixSet(float4x4._11, float4x4._12, float4x4._13, float4x4._14,
                                  float4x4._21, float4x4._22, float4x4._23, float4x4._24,
                                  float4x4._31, float4x4._32, float4x4._33, float4x4._34,
                                  0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR determinant = XMMatrixDeterminant(matrix);
    matrix = XMMatrixTranspose(XMMatrixInverse(&determinant, matrix));

    XMFLOAT3X4 float3x4;
    XMStoreFloat3x4(&float3x4, matrix);
    return float3x4;
}

//----------------------------------------------------------------------------------------------------------------------

#ifdef __clang__
#pragma clang diagnostic pop
#endif