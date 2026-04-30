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
        constexpr size_t serialized_assets_header_size = sizeof(uint32_t) * 3;
        struct ImageMetadata
        {
            ImageFormat format;
            uint8_t persistent = 0;
            char name[metadata_name_size];
            char filepath[metadata_filepath_size];
        };
        constexpr size_t image_metadata_serialized_size = sizeof(ImageFormat) +
            sizeof(uint8_t) +
            metadata_name_size +
            metadata_filepath_size;

        struct TextureMetadata
        {
            TextureSamplerFilterMode filterMode;
            TextureSamplerAddressMode addressMode;
            uint8_t useMipmapping = 0;
            uint8_t persistent = 0;
            // TODO: reserve certain range or IDs for engine external asset IDs, so this is less
            // likely get fucked!
            ID_t imageID = NULL_ID;
            char name[metadata_name_size];
        };
        constexpr size_t texture_metadata_serialized_size = sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint8_t) * 2 +
            sizeof(ID_t) +
            metadata_name_size;

        struct MaterialMetadata
        {
            float specularStrength = 0.0f;
            float shininess = 0.0f;
            Vector2f textureOffset;
            Vector2f textureScale;
            uint8_t castShadows = 1;
            uint8_t receiveShadows = 1;
            uint8_t transparent = 0;
            uint8_t shadeless = 0;
            uint8_t persistent = 0;
            ID_t blendmapTextureID = NULL_ID;
            ID_t diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            ID_t specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            ID_t normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            char name[metadata_name_size];
        };

        constexpr size_t material_metadata_serialized_size = sizeof(float) * 2 +
            sizeof(Vector2f) * 2 +
            sizeof(uint8_t) * 5 +
            sizeof(ID_t) +
            sizeof(ID_t) * PE_MAX_MATERIAL_TEX_CHANNELS * 3 +
            metadata_name_size;

        std::string image_metadata_to_string(const ImageMetadata& data);
        std::string texture_metadata_to_string(const TextureMetadata& data);
        std::string material_metadata_to_string(const MaterialMetadata& data);

        ImageMetadata get_image_metadata(const Image* pImage);
        TextureMetadata get_texture_metadata(const Texture* pTexture);
        MaterialMetadata get_material_metadata(const Material* pMaterial);

        std::vector<char> serialize_image_metadata(ImageMetadata data);
        std::vector<char> serialize_texture_metadata(TextureMetadata data);
        std::vector<char> serialize_material_metadata(MaterialMetadata data);

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData);
        TextureMetadata deserialize_texture_metadata(size_t dataSize, void* pData);
        MaterialMetadata deserialize_material_metadata(size_t dataSize, void* pData);

        std::vector<char> serialize_assets(const std::vector<Asset*>& assets);
        void deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures,
            std::vector<MaterialMetadata>& outMaterials
        );
    }
}
