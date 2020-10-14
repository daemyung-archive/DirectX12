//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef UTILITY_H_
#define UTILITY_H_

#include <Windows.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <fmt/format.h>
#include <stdexcept>
#include <tuple>
#include <filesystem>

//----------------------------------------------------------------------------------------------------------------------

static const auto kXAxisVector = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
static const auto kYAxisVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
static const auto kZAxisVector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
static const DirectX::XMFLOAT3 kZeroFloat3 = {0.0f, 0.0f, 0.0f};
static const DirectX::XMFLOAT4X4 kIdentityFloat4x4 = {1.0f, 0.0f, 0.0f, 0.0f,
                                                      0.0f, 1.0f, 0.0f, 0.0f,
                                                      0.0f, 0.0f, 1.0f, 0.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f};

//----------------------------------------------------------------------------------------------------------------------

using Resolution = std::tuple<UINT, UINT>;

//----------------------------------------------------------------------------------------------------------------------

//! Retrieve a width from a resolution.
//! \param resolution A resolution.
//! \return A width.
inline auto GetWidth(const Resolution &resolution) {
    return std::get<0>(resolution);
}

//----------------------------------------------------------------------------------------------------------------------

//! Retrieve a height from a resolution.
//! \param resolution A resolution.
//! \return A height.
inline auto GetHeight(const Resolution &resolution) {
    return std::get<1>(resolution);
}

//----------------------------------------------------------------------------------------------------------------------

//! Retrieve an aspect ratio of a resolution.
//! \param resolution A resolution.
//! \return An aspect ratio.
inline auto GetAspectRatio(const Resolution &resolution) {
    return GetWidth(resolution) / static_cast<float>(GetHeight(resolution));
}

//----------------------------------------------------------------------------------------------------------------------

//! Rounds the specified value up to the nearest value meeting the specified alignment.
//! Only power of 2 alignments are supported by this function.
//! \param value A value.
//! \param alignment A power of 2 alignment.
//! \return An aligned value.
template<typename T>
T AlignPow2(T value, UINT64 alignment) {
    return ((value + static_cast<T>(alignment) - 1) & ~(static_cast<T>(alignment) - 1));
}

//----------------------------------------------------------------------------------------------------------------------

//! Convert from UTF16 to UTF8.
//! \param utf16 A UTF16 string.
//! \return A UTF8 string.
extern std::string ConvertUTF16ToUTF8(wchar_t *utf16);

//----------------------------------------------------------------------------------------------------------------------

//! Read a file.
//! \param path The file path.
//! \return A contents of file.
extern std::vector<BYTE> ReadFile(const std::filesystem::path &path);

//----------------------------------------------------------------------------------------------------------------------

//! Throw an exception if a function returns an error.
//! \param function A function call.
#define ThrowIfFailed(function) {                                                              \
    HRESULT result = function;                                                                 \
    if (FAILED(result)) {                                                                      \
        throw std::runtime_error(fmt::format("{}({}): error {:#08x}: {} ", __FILE__, __LINE__, \
            static_cast<UINT>(result), #function));                                            \
    }                                                                                          \
}

//----------------------------------------------------------------------------------------------------------------------

//! Compile a shader.
//! \param file_path The name of file path that contains the shader code.
//! \param entrypoint The name of shader entrypoint function where shader execution begin.
//! \param target The shader target or set of shader features to compile against.
//! \param code A pointer to a variable that receives a pointer to ID3DBlob.
//! \return A result.
[[maybe_unused]]
extern HRESULT CompileShader(const std::filesystem::path &file_path, const std::string &entrypoint,
                             const std::string &target, ID3DBlob **code);

//----------------------------------------------------------------------------------------------------------------------

//! Compile a shader.
//! \param file_path The name of file path that contains the shader code.
//! \param defines An optional array that define shader macros.
//! \param entrypoint The name of shader entrypoint function where shader execution begin.
//! \param target The shader target or set of shader features to compile against.
//! \param code A pointer to a variable that receives a pointer to ID3DBlob.
//! \return A result.
[[maybe_unused]]
extern HRESULT CompileShader(const std::filesystem::path &file_path, const D3D_SHADER_MACRO *defines,
                             const std::string &entrypoint, const std::string &target, ID3DBlob **code);

//----------------------------------------------------------------------------------------------------------------------

//! Create a serialize root signature.
//! \param desc A description of root signature.
//! \param blob A pointer to a variable that receives a pointer to ID3DBlob.
//! \return A result.
[[maybe_unused]]
extern HRESULT SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *desc, ID3DBlob **blob);

//----------------------------------------------------------------------------------------------------------------------

//! Create a root signature.
//! \param device A DirectX12 device.
//! \param desc
//! \param root_signature A pointer to a variable that
//! \return A result.
[[maybe_unused]]
extern HRESULT CreateRootSignature(ID3D12Device *device, const D3D12_ROOT_SIGNATURE_DESC *desc,
                                   ID3D12RootSignature **root_signature);

//----------------------------------------------------------------------------------------------------------------------

//! Create a buffer which has the default heap.
//! \param device A DirectX12 device.
//! \param size The byte size of a buffer.
//! \param buffer A pointer to a memory block that receives a pointer to ID3D12Resource.
//! \return A result.
[[maybe_unused]]
extern HRESULT CreateDefaultBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer);

//----------------------------------------------------------------------------------------------------------------------

//! Create a buffer which has the upload heap.
//! \param device A DirectX12 device.
//! \param size The byte size of a buffer.
//! \param buffer A pointer to a memory block that receives a pointer to ID3D12Resource.
//! \return A result.
[[maybe_unused]]
extern HRESULT CreateUploadBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer);

//----------------------------------------------------------------------------------------------------------------------

//! Update the data to a buffer. A buffer must be mappable.
//! \param buffer A buffer will be initialized.
//! \param data The data to be initialized to a buffer.
//! \param size The byte size of a buffer.
extern void UpdateBuffer(ID3D12Resource *buffer, void *data, UINT64 size);

//----------------------------------------------------------------------------------------------------------------------

//! Create a constant buffer which has the upload heap and aligned size by 256.
//! \param device A DirectX12 device.
//! \param size The byte size of a buffer. It will be aligned by 256.
//! \param buffer A pointer to a memory block that receives a pointer to ID3D12Resource.
//! \return A result.
extern HRESULT CreateConstantBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer);

//----------------------------------------------------------------------------------------------------------------------

//! Create a default texture 2D.
//! \param device A DirectX12 device.
//! \param width The width of a texture.
//! \param height The height of a texture.
//! \param mip_levels The mip levels of a texture.
//! \param format The texture format.
//! \param buffer A pointer to a memory block that receives a pointer to ID3D12Resource.
//! \return A result.
extern HRESULT CreateDefaultTexture2D(ID3D12Device *device, UINT64 width, UINT height, UINT16 mip_levels,
                                      DXGI_FORMAT format, ID3D12Resource **buffer);

//----------------------------------------------------------------------------------------------------------------------

//! Calculate the inverse transpose matrix.
//! \param float4x4 A matrix for calculating the inverse transpose matrix.
//! \return A inverse transpose matrix.
extern DirectX::XMFLOAT3X4 XMMatrixInverseTranspose(const DirectX::XMFLOAT4X4 &float4x4);

//----------------------------------------------------------------------------------------------------------------------

#endif