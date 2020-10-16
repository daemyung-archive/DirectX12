//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef IMAGE_LOADER_H_
#define IMAGE_LOADER_H_

#include <Windows.h>
#include <dxgiformat.h>
#include <vector>
#include <filesystem>

//----------------------------------------------------------------------------------------------------------------------

struct Subresource {
    const BYTE* data;
    UINT64 row_pitch;
    UINT height;
};

//----------------------------------------------------------------------------------------------------------------------

struct Image {
    std::vector<BYTE> contents;
    UINT64 width = 0;
    UINT height = 0;
    UINT16 array_size = 0;
    UINT16 mip_levels = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    std::vector<Subresource> subresources;
};

//----------------------------------------------------------------------------------------------------------------------

class ImageLoader final {
public:
    //! Load an image from file.
    //! \param path A file path.
    //! \return An image.
    Image LoadFile(const std::filesystem::path &path);
};

//----------------------------------------------------------------------------------------------------------------------

#endif