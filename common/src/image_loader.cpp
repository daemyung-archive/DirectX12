//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#define DDSKTX_IMPLEMENT

#include "image_loader.h"

#include <d3dx12.h>
#include <dds-ktx.h>

#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

auto CastToFormat(ddsktx_format format) {
    switch (format) {
        case DDSKTX_FORMAT_RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            throw std::runtime_error("Fail to cast to format.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

Image ImageLoader::LoadFile(const std::filesystem::path &path) {
    auto extension = path.extension();

    if (extension == ".ktx" || extension == ".dds") {
        return LoadDDSKTX(path);
    }

    throw std::runtime_error("");
}

//----------------------------------------------------------------------------------------------------------------------

Image ImageLoader::LoadDDSKTX(const std::filesystem::path &path) {
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