//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#define DDSKTX_IMPLEMENT
#define STB_IMAGE_IMPLEMENTATION

#include "image_loader.h"

#include <d3dx12.h>
#include <dds-ktx.h>
#include <stb_image.h>

#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

//! Cast from the DDSKTX format to the DirectX12 format.
//! \param format The DDSKTX format.
//! \return The DirectX12 format.
auto CastToFormat(ddsktx_format format) {
    switch (format) {
        case DDSKTX_FORMAT_RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DDSKTX_FORMAT_BGRA8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DDSKTX_FORMAT_BC7:
            return DXGI_FORMAT_BC7_UNORM;
        default:
            throw std::runtime_error("Fail to cast to format.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

//! Load an image from DDS or KTX file.
//! \param path A file path.
//! \return An image.
Image LoadDDSKTX(const std::filesystem::path &path) {
    Image image;

    // Read pixels from a file.
    image.contents = ReadFile(path);
    auto size = static_cast<INT>(image.contents.size());

    // Read texture information.
    ddsktx_error error;
    ddsktx_texture_info info;
    if (!ddsktx_parse(&info, image.contents.data(), size, &error)) {
        throw std::runtime_error(fmt::format("Fail to parse {}: {}.", path.string(), error.msg));
    }

    image.width = info.width;
    image.height = info.height;
    image.array_size = info.num_layers;
    image.mip_levels = info.num_mips;
    image.format = CastToFormat(info.format);

    // Read sub data of a texture.
    image.subresources.resize(info.num_layers * info.num_mips);
    for (auto layer = 0; layer != info.num_layers; ++layer) {
        for (auto mip = 0; mip != info.num_mips; ++mip) {
            // Read sub data.
            ddsktx_sub_data sub_data;
            ddsktx_get_sub(&info, &sub_data, image.contents.data(), size, layer, 0, mip);

            // Fill subresource.
            auto index = D3D12CalcSubresource(mip, layer, 0, info.num_mips, info.num_layers);
            auto &subresource = image.subresources[index];
            subresource.data = static_cast<const BYTE *>(sub_data.buff);
            subresource.row_pitch = sub_data.row_pitch_bytes;
            subresource.height = sub_data.height;
        }
    }

    return image;
}

//----------------------------------------------------------------------------------------------------------------------

//! Load an image from STB file.
//! \param path A file path.
//! \return An image.
Image LoadSTB(const std::filesystem::path &path) {
    Image image;

    // Read pixels from a file.
    int x, y, channels;
    auto contents = stbi_load(path.string().c_str(), &x, &y, &channels, STBI_rgb_alpha);
    auto row_pitch = x * STBI_rgb_alpha;
    auto size = row_pitch * y;

    image.contents.resize(size);
    memcpy(image.contents.data(), contents, size);
    stbi_image_free(contents);

    image.width = x;
    image.height = y;
    image.array_size = 1;
    image.mip_levels = 1;
    image.format = DXGI_FORMAT_R8G8B8A8_UNORM;

    Subresource subresource;
    subresource.data = image.contents.data();
    subresource.row_pitch = row_pitch;
    subresource.height = y;

    image.subresources.push_back(subresource);

    return image;
}

//----------------------------------------------------------------------------------------------------------------------

Image ImageLoader::LoadFile(const std::filesystem::path &path) {
    auto extension = path.extension();

    if (extension == ".ktx" || extension == ".dds") {
        return LoadDDSKTX(path);
    } else if (extension == ".png") {
        return LoadSTB(path);
    }

    throw std::runtime_error("");
}

//----------------------------------------------------------------------------------------------------------------------
