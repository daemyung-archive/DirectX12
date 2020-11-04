//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef COMPILER_H_
#define COMPILER_H_

#include <wrl.h>
#include <Windows.h>
#include <dxcapi.h>
#include <filesystem>

//----------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------------------------------------

class Compiler final {
public:
    //! Constructor.
    Compiler();

    //! Compile a shader
    //! \param path The file path that contains the shader code.
    //! \param entrypoint The name of shader entrypoint function where shader execution begin.
    //! \param target The shader target or set of shader features to compile against.
    //! \param code A pointer to a variable that receives a pointer to IDxcBlob.
    //! \return A result.
    HRESULT CompileShader(const std::filesystem::path &path, const std::wstring &entrypoint,
                          const std::wstring &target, IDxcBlob** code);

    //! Compile a library.
    //! \param path The file path that contains the shader code.
    //! \param code The shader target or set of shader features to compile against.
    //! \return A result.
    HRESULT CompileLibrary(const std::filesystem::path &path, IDxcBlob** code);

private:
    //! Initialize DLLs.
    void InitDLLs();

    //! Initialize compiler and library.
    void InitCompilerAndLibrary();

private:
    HMODULE _dxil = 0;
    HMODULE _dxcompiler = 0;
    ComPtr<IDxcCompiler> _compiler;
    ComPtr<IDxcLibrary> _library;
    ComPtr<IDxcIncludeHandler> _include_handler;
};

//----------------------------------------------------------------------------------------------------------------------

#endif