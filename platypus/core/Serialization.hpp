#pragma once

#include "platypus/assets/Image.hpp"
#include "platypus/assets/Texture.hpp"
#include "platypus/utils/Maths.hpp"
#include <vector>


namespace platypus
{
    namespace serialization
    {
        constexpr size_t metadata_name_size = 32;
        constexpr size_t metadata_filepath_size = 64;
        struct ImageMetadata
        {
            ImageFormat format;
            uint32_t persistent = 0;
            char name[metadata_name_size];
            char filepath[metadata_filepath_size];
        };

        struct TextureMetadata
        {
            Vector2f offset;
            Vector2f scale;
            TextureSamplerFilterMode filterMode;
            TextureSamplerAddressMode addressMode;
            uint32_t useMipmapping = 0;
            uint32_t persistent = 0;
            char name[metadata_name_size];
            char image[metadata_name_size];
        };

        // Size without possible struct mem padding
        constexpr size_t image_metadata_file_size = sizeof(ImageFormat) +
            sizeof(uint32_t) +
            metadata_name_size +
            metadata_filepath_size;

        constexpr size_t texture_metadata_file_size = sizeof(Vector2f) * 2 +
            sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint32_t) * 2 +
            metadata_name_size * 2;

        ImageMetadata get_image_metadata(const Image* pImage, const std::string& assetName);

        std::vector<char> serialize_image_metadata(ImageMetadata data);
        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData);
    }
}
