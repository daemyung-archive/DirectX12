//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "compiler.h"

#include <fmt/format.h>
#include <stdexcept>

#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

auto BuildLibraryPath(const std::string &name) {
    return ConvertUTF8ToUTF16(fmt::format("{}/{}", EXTERNAL_LIBRARY_DIR, name).c_str());
}

//----------------------------------------------------------------------------------------------------------------------

Compiler::Compiler() {
    InitDLLs();
    InitCompilerAndLibrary();
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT Compiler::CompileShader(const std::filesystem::path &path, const std::wstring &entrypoint,
                                const std::wstring &target, IDxcBlob** code) {
    auto source = ReadFile(path);

    // Create encoded source from the string.
    ComPtr<IDxcBlobEncoding> encoded_source;
    ThrowIfFailed(_library->CreateBlobWithEncodingFromPinned(source.data(), static_cast<UINT32>(source.size()),
                                                             CP_UTF8, &encoded_source));

    // Configure compile options.
    const wchar_t *options[] = {
            L"-WX",           //Warnings as errors.
#ifdef _DEBUG
            L"-Zi",           // Debug info.
            L"-Qembed_debug", // Embed debug info into the shader.
            L"-Od",           // Disable optimization.
#else
            L"-O3",           // Optimization level 3.
#endif
    };

    // Compile a shader.
    ComPtr<IDxcOperationResult> operation_result;
    ThrowIfFailed(_compiler->Compile(encoded_source.Get(), path.c_str(), entrypoint.c_str(), target.c_str(),
                                     options, _countof(options),
                                     nullptr, 0,
                                     _include_handler.Get(), &operation_result));

    // Verify the result.
    HRESULT result;
    operation_result->GetStatus(&result);

    if (FAILED(result)) {
        ComPtr<IDxcBlobEncoding> error;
        operation_result->GetErrorBuffer(&error);
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    } else {
        operation_result->GetResult(code);
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

HRESULT Compiler::CompileLibrary(const std::filesystem::path &path, IDxcBlob** code) {
    return CompileShader(path, L"", L"lib_6_3", code);
}

//----------------------------------------------------------------------------------------------------------------------

void Compiler::InitDLLs() {
    // dxil should be loaded before loading dxcompiler.
    // If this rule isn't keep, the compiler can't generate the intermediate language.
    _dxil = LoadLibrary(BuildLibraryPath("dxil.dll").c_str());
    if (!_dxil) {
        throw std::runtime_error(fmt::format("Fail to load {}.", "dxil.dll"));
    }

    _dxcompiler = LoadLibrary(BuildLibraryPath("dxcompiler.dll").c_str());
    if (!_dxcompiler) {
        throw std::runtime_error(fmt::format("Fail to load {}.", "dxcompiler.dll"));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Compiler::InitCompilerAndLibrary() {
    auto proc = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(_dxcompiler, "DxcCreateInstance"));
    if (!proc) {
        throw std::runtime_error("Fail to get DxcCreateInstance.");
    }

    ThrowIfFailed(proc(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
    ThrowIfFailed(proc(CLSID_DxcLibrary, IID_PPV_ARGS(&_library)));
    ThrowIfFailed(_library->CreateIncludeHandler(&_include_handler));
}

//----------------------------------------------------------------------------------------------------------------------
