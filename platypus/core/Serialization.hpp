#pragma once

#include "platypus/assets/Image.hpp"
#include "platypus/assets/Texture.hpp"
#include "platypus/assets/AssetManager.hpp"
#include "platypus/utils/Maths.hpp"
#include <vector>


namespace platypus
{
    namespace serialization
    {
        constexpr size_t metadata_name_size = 32;
        constexpr size_t metadata_filepath_size = 64;
        constexpr size_t serialized_assets_header_size = sizeof(uint32_t) * 2;
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
            // TODO: reserve certain range or IDs for engine external asset IDs, so this is less
            // likely get fucked!
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

        ImageMetadata get_image_metadata(const Image* pImage);
        TextureMetadata get_texture_metadata(const Texture* pTexture);

        std::vector<char> serialize_image_metadata(ImageMetadata data);
        std::vector<char> serialize_texture_metadata(TextureMetadata data);

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData);
        TextureMetadata deserialize_texture_metadata(size_t dataSize, void* pData);

        std::vector<char> serialize_assets(const std::vector<Asset*>& assets);
        void deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures
        );
    }
}
