//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef UTILITY_H_
#define UTILITY_H_

#include <Windows.h>
#include <d3d12.h>
#include <fmt/format.h>
#include <stdexcept>
#include <tuple>
#include <filesystem>

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

//! Convert from UTF16 to UTF8.
//! \param utf16 A UTF16 string.
//! \return A UTF8 string.
std::string ConvertUTF16ToUTF8(wchar_t *utf16);

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
//! \return A result
[[maybe_unused]]
extern HRESULT CreateUploadBuffer(ID3D12Device *device, UINT64 size, ID3D12Resource **buffer);

//----------------------------------------------------------------------------------------------------------------------

//! Update the data to a buffer. A buffer must be mappable.
//! \param buffer A buffer will be initialized.
//! \param data The data to be initialized to a buffer.
//! \param size The byte size of a buffer.
extern void UpdateBuffer(ID3D12Resource *buffer, void *data, UINT64 size);

//----------------------------------------------------------------------------------------------------------------------

#endif