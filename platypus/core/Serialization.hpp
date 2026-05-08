#pragma once

#include "platypus/assets/Image.hpp"
#include "platypus/assets/Texture.hpp"
#include "platypus/assets/Material.hpp"
#include "platypus/assets/Model.hpp"
#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"
#include <vector>


namespace platypus
{
    namespace serialization
    {
        constexpr size_t metadata_name_size = 32;
        constexpr size_t metadata_filepath_size = 64;
        constexpr size_t metadata_model_max_meshes = 8;
        constexpr size_t serialized_assets_header_size = sizeof(uint32_t) * 4;

        // TODO:
        // *Add asset ID to all metadata structs!
        // *Some way to make sure that engine's "internal asset IDs" can never conflict with
        // user generated asset IDs!
        struct ImageMetadata
        {
            ID_t assetID = NULL_ID;
            ImageFormat format;
            uint8_t persistent = 0;
            char name[metadata_name_size];
            char filepath[metadata_filepath_size];
        };
        constexpr size_t image_metadata_serialized_size =
            sizeof(ID_t) +
            sizeof(ImageFormat) +
            sizeof(uint8_t) +
            metadata_name_size +
            metadata_filepath_size;

        struct TextureMetadata
        {
            ID_t assetID = NULL_ID;
            ID_t imageID = NULL_ID;
            TextureSamplerFilterMode filterMode;
            TextureSamplerAddressMode addressMode;
            uint8_t useMipmapping = 0;
            uint8_t persistent = 0;
            char name[metadata_name_size];
        };
        constexpr size_t texture_metadata_serialized_size =
            sizeof(ID_t) * 2 +
            sizeof(TextureSamplerFilterMode) +
            sizeof(TextureSamplerAddressMode) +
            sizeof(uint8_t) * 2 +
            metadata_name_size;

        // TODO: Add custom shader filepaths to MaterialMetadata!
        struct MaterialMetadata
        {
            ID_t assetID = NULL_ID;
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

        constexpr size_t material_metadata_serialized_size =
            sizeof(ID_t) +
            sizeof(float) * 2 +
            sizeof(Vector2f) * 2 +
            sizeof(uint8_t) * 5 +
            sizeof(ID_t) +
            sizeof(ID_t) * PE_MAX_MATERIAL_TEX_CHANNELS * 3 +
            metadata_name_size;

        // TODO: Some more coherent and centralized thing for current metadata_model_max_meshes!
        struct ModelMetadata
        {
            ID_t assetID = NULL_ID;
            uint8_t instanced = 0;
            uint8_t persistent = 0;
            ID_t meshIDs[metadata_model_max_meshes];
            char name[metadata_name_size];
            char filepath[metadata_filepath_size];
        };

        constexpr size_t model_metadata_serialized_size =
            sizeof(ID_t) +
            sizeof(uint8_t) * 2 +
            sizeof(ID_t) * metadata_model_max_meshes +
            metadata_name_size +
            metadata_filepath_size;

        std::string image_metadata_to_string(const ImageMetadata& data);
        std::string texture_metadata_to_string(const TextureMetadata& data);
        std::string material_metadata_to_string(const MaterialMetadata& data);
        std::string model_metadata_to_string(const ModelMetadata& data);

        ImageMetadata get_image_metadata(const Image* pImage);
        TextureMetadata get_texture_metadata(const Texture* pTexture);
        MaterialMetadata get_material_metadata(const Material* pMaterial);
        ModelMetadata get_model_metadata(const Model* pModel);

        std::vector<char> serialize_image_metadata(ImageMetadata data);
        std::vector<char> serialize_texture_metadata(TextureMetadata data);
        std::vector<char> serialize_material_metadata(MaterialMetadata data);
        std::vector<char> serialize_model_metadata(ModelMetadata data);

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData);
        TextureMetadata deserialize_texture_metadata(size_t dataSize, void* pData);
        MaterialMetadata deserialize_material_metadata(size_t dataSize, void* pData);
        ModelMetadata deserialize_model_metadata(size_t dataSize, void* pData);

        std::vector<char> serialize_assets(const std::vector<Asset*>& assets);
        std::vector<char> serialize_entities(const Scene* pScene, const std::vector<entityID_t>& entities);
        size_t deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures,
            std::vector<MaterialMetadata>& outMaterials,
            std::vector<ModelMetadata>& outModels
        );
        void deserialize_entities(
            Scene* pScene,
            void* pData,
            std::vector<entityID_t>& outEntities
        );

        void write(
            const Scene* pScene,
            const std::string& filepath,
            const std::vector<Asset*>& assets,
            const std::vector<entityID_t>& entities
        );

        void read(
            Scene* pScene,
            const std::string& filepath,
            std::vector<serialization::ImageMetadata>& outImages,
            std::vector<serialization::TextureMetadata>& outTextures,
            std::vector<serialization::MaterialMetadata>& outMaterials,
            std::vector<serialization::ModelMetadata>& outModels,
            std::vector<entityID_t>& outEntities
        );
    }
}
