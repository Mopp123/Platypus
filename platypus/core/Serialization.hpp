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
            TextureSamplerFilterMode filterMode;
            TextureSamplerAddressMode addressMode;
            uint32_t useMipmapping = 0;
            uint32_t persistent = 0;
            ID_t imageID = NULL_ID;
            char name[metadata_name_size];
        };

        // Size without possible struct mem padding
        constexpr size_t image_metadata_file_size = sizeof(ImageFormat) +
            sizeof(uint32_t) +
            metadata_name_size +
            metadata_filepath_size;

        constexpr size_t texture_metadata_file_size = sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint32_t) * 2 +
            sizeof(ID_t) +
            metadata_name_size;

        ImageMetadata get_image_metadata(const Image* pImage, const std::string& assetName);
        //TextureMetadata get_texture_metadata(const Texture* pTexture, const std::string& assetName);

        std::vector<char> serialize_image_metadata(ImageMetadata data);
        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData);
    }
}
