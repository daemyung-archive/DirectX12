//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef UTILITY_H_
#define UTILITY_H_

#include <Windows.h>
#include <fmt/format.h>
#include <stdexcept>
#include <tuple>

//----------------------------------------------------------------------------------------------------------------------

using Resolution = std::tuple<UINT, UINT>;

//----------------------------------------------------------------------------------------------------------------------

/// Retrieve a width from a resolution.
/// \param resolution A resolution.
/// \return A width.
inline auto GetWidth(const Resolution &resolution) {
    return std::get<0>(resolution);
}

//----------------------------------------------------------------------------------------------------------------------

/// Retrieve a height from a resolution.
/// \param resolution A resolution.
/// \return A height.
inline auto GetHeight(const Resolution &resolution) {
    return std::get<1>(resolution);
}

//----------------------------------------------------------------------------------------------------------------------

/// Throw an exception if a function returns an error.
/// \param function A function call.
#define ThrowIfFailed(function) {                                                              \
    HRESULT result = function;                                                                 \
    if (FAILED(result)) {                                                                      \
        throw std::runtime_error(fmt::format("{}({}): error {:#08x}: {} ", __FILE__, __LINE__, \
            static_cast<UINT>(result), #function));                                            \
    }                                                                                          \
}

//----------------------------------------------------------------------------------------------------------------------

#endif