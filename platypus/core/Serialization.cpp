#include "Serialization.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include <string>
#include <cstring>
#include <fstream>


namespace platypus
{
    namespace serialization
    {
        std::string image_metadata_to_string(const ImageMetadata& data)
        {
            std::string name(data.name);
            std::string filepath(data.filepath);
            return name + "\n"
                "   ID = " + std::to_string(data.assetID) + "\n"
                "   format = " + image_format_to_string(data.format) + "\n"
                "   filepath = " + filepath + "\n"
                "   persistent = " + std::to_string(data.persistent);
        }

        std::string texture_metadata_to_string(const TextureMetadata& data)
        {
            std::string name(data.name);
            return name + "\n"
                "   ID = " + std::to_string(data.assetID) + "\n"
                "   imageID = " + std::to_string(data.imageID) + "\n"
                "   filterMode = " + texture_sampler_filter_mode_to_string(data.filterMode) + "\n"
                "   addressMode = " + texture_sampler_address_mode_to_string(data.addressMode) + "\n"
                "   useMipmapping = " + std::to_string(data.useMipmapping) + "\n"
                "   persistent = " + std::to_string(data.persistent);
        }

        std::string material_metadata_to_string(const MaterialMetadata& data)
        {
            std::string name(data.name);

            std::string str = name + "\n"
                "   ID = " + std::to_string(data.assetID) + "\n"
                "   specularStrength = " + std::to_string(data.specularStrength) + "\n"
                "   specularShininess = " + std::to_string(data.shininess) + "\n"
                "   textureOffset = " + data.textureOffset.toString() + "\n"
                "   textureScale = " + data.textureScale.toString() + "\n"
                "   castShadows = " + std::to_string(data.castShadows) + "\n"
                "   receiveShadows = " + std::to_string(data.receiveShadows) + "\n"
                "   transparent = " + std::to_string(data.transparent) + "\n"
                "   shadeless = " + std::to_string(data.shadeless) + "\n"
                "   blendmapTextureID = " + std::to_string(data.blendmapTextureID) + "\n"
                "   diffuseTextureIDs\n";

            for (size_t i = 0; i < PE_MAX_MATERIAL_TEX_CHANNELS; ++i)
                str += "        [" + std::to_string(i) + "] = " + std::to_string(data.diffuseTextureIDs[i]) + "\n";

            str += "    diffuseTextureIDs\n";
            for (size_t i = 0; i < PE_MAX_MATERIAL_TEX_CHANNELS; ++i)
                str += "        [" + std::to_string(i) + "] = " + std::to_string(data.specularTextureIDs[i]) + "\n";

            str += "    normalTextureIDs\n";
            for (size_t i = 0; i < PE_MAX_MATERIAL_TEX_CHANNELS; ++i)
                str += "        [" + std::to_string(i) + "] = " + std::to_string(data.normalTextureIDs[i]) + "\n";

            str += "    persistent = " + std::to_string(data.persistent);

            return str;
        }

        std::string model_metadata_to_string(const ModelMetadata& data)
        {
            std::string name(data.name);
            std::string filepath(data.filepath);

            std::string str = name + "\n";
            str += "    ID = " + std::to_string(data.assetID) + "\n";
            str += "    filepath = " + filepath + "\n";
            str += "    instanced = " + std::to_string(data.instanced) + "\n";

            str += "    meshIDs\n";
            for (size_t i = 0; i < metadata_model_max_meshes; ++i)
                str += "        [" + std::to_string(i) + "] = " + std::to_string(data.meshIDs[i]) + "\n";

            str += "    persistent = " + std::to_string(data.persistent);
            return str;
        }

        ImageMetadata get_image_metadata(const Image* pImage)
        {
            const std::string& assetName = pImage->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            const std::string& filepath = pImage->getFilepath();
            PLATYPUS_ASSERT(filepath.size() < metadata_filepath_size);

            ImageMetadata data;
            data.assetID = pImage->getID();
            data.format = pImage->getFormat();
            data.persistent = static_cast<uint8_t>(pImage->isPersistent());
            memset(data.name, 0, metadata_name_size);
            memset(data.filepath, 0, metadata_filepath_size);
            memcpy(data.name, assetName.data(), assetName.size());
            memcpy(data.filepath, filepath.data(), filepath.size());
            return data;
        }

        TextureMetadata get_texture_metadata(const Texture* pTexture)
        {
            const std::string& assetName = pTexture->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            PLATYPUS_ASSERT(pTexture->getID() != NULL_ID);
            PLATYPUS_ASSERT(pTexture->getImage());
            PLATYPUS_ASSERT(pTexture->getImage()->getID() != NULL_ID);

            const TextureSampler* pSampler = pTexture->getTextureSampler();
            TextureMetadata data;
            data.assetID = pTexture->getID();
            data.imageID = pTexture->getImage()->getID();
            data.filterMode = pSampler->getFilterMode();
            data.addressMode = pSampler->getAddressMode();
            data.useMipmapping = static_cast<uint8_t>(pSampler->isMipmapped());
            data.persistent = static_cast<uint8_t>(pTexture->isPersistent());
            memset(data.name, 0, metadata_name_size);
            memcpy(data.name, assetName.data(), assetName.size());
            return data;
        }

        MaterialMetadata get_material_metadata(const Material* pMaterial)
        {
            const std::string& assetName = pMaterial->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;

            MaterialMetadata data;
            data.assetID = pMaterial->getID();
            data.specularStrength = pMaterial->getSpecularStrength();
            data.shininess = pMaterial->getShininess();
            data.textureOffset = pMaterial->getTextureOffset();
            data.textureScale = pMaterial->getTextureScale();
            data.castShadows = static_cast<uint8_t>(pMaterial->castsShadows());
            data.receiveShadows = static_cast<uint8_t>(pMaterial->receivesShadows());
            data.transparent = static_cast<uint8_t>(pMaterial->isTransparent());
            data.shadeless = static_cast<uint8_t>(pMaterial->isShadeless());
            data.persistent = static_cast<uint8_t>(pMaterial->isPersistent());
            data.blendmapTextureID = pMaterial->getBlendmapTextureID();

            memset(data.diffuseTextureIDs, NULL_ID, sizeof(ID_t) * texturesPerChannel);
            memset(data.specularTextureIDs, NULL_ID, sizeof(ID_t) * texturesPerChannel);
            memset(data.normalTextureIDs, NULL_ID, sizeof(ID_t) * texturesPerChannel);
            memset(data.name, NULL_ID, metadata_name_size);

            const size_t channelTexturesSize = sizeof(ID_t) * texturesPerChannel;
            memcpy(
                data.diffuseTextureIDs,
                pMaterial->getDiffuseTextureIDs(),
                channelTexturesSize
            );
            memcpy(
                data.specularTextureIDs,
                pMaterial->getSpecularTextureIDs(),
                channelTexturesSize
            );
            memcpy(
                data.normalTextureIDs,
                pMaterial->getNormalTextureIDs(),
                channelTexturesSize
            );

            memcpy(data.name, assetName.data(), assetName.size());

            return data;
        }

        ModelMetadata get_model_metadata(const Model* pModel)
        {
            const std::string& assetName = pModel->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            const std::string& filepath = pModel->getFilepath();
            PLATYPUS_ASSERT(filepath.size() < metadata_filepath_size);

            ModelMetadata data;
            data.assetID = pModel->getID();
            data.instanced = static_cast<bool>(pModel->isInstanced());
            data.persistent = static_cast<bool>(pModel->isPersistent());

            memset(data.meshIDs, NULL_ID, sizeof(ID_t) * metadata_model_max_meshes);
            memset(data.name, 0, metadata_name_size);
            memset(data.filepath, 0, metadata_filepath_size);

            const std::vector<Mesh*> meshes = pModel->getMeshes();
            for (size_t i = 0; i < meshes.size(); ++i)
            {
                ID_t meshID = meshes[i]->getID();
                memcpy(data.meshIDs + i, &meshID, sizeof(ID_t));
            }

            memcpy(data.name, assetName.data(), assetName.size());
            memcpy(data.filepath, filepath.data(), filepath.size());

            return data;
        }

        std::vector<char> serialize_image_metadata(ImageMetadata data)
        {
            std::vector<char> serializedData(image_metadata_serialized_size);
            memcpy(
                serializedData.data(),
                reinterpret_cast<void*>(&data.assetID),
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.format),
                sizeof(ImageFormat)
            );
            pos += sizeof(ImageFormat);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.persistent),
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.name),
                metadata_name_size
            );
            pos += metadata_name_size;

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.filepath),
                metadata_filepath_size
            );

            return serializedData;
        }

        std::vector<char> serialize_texture_metadata(TextureMetadata data)
        {
            std::vector<char> serializedData(texture_metadata_serialized_size);
            PLATYPUS_ASSERT(data.assetID != NULL_ID);

            memcpy(
                serializedData.data(),
                reinterpret_cast<void*>(&data.assetID),
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.imageID),
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.filterMode),
                sizeof(TextureSamplerFilterMode)
            );
            pos += sizeof(TextureSamplerFilterMode);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.addressMode),
                sizeof(TextureSamplerAddressMode)
            );
            pos += sizeof(TextureSamplerAddressMode);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.useMipmapping),
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.persistent),
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                data.name,
                metadata_name_size
            );
            pos += metadata_name_size;
            PLATYPUS_ASSERT(pos == texture_metadata_serialized_size);

            return serializedData;
        }

        std::vector<char> serialize_material_metadata(MaterialMetadata data)
        {
            std::vector<char> serializedData(material_metadata_serialized_size);
            const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;

            memcpy(
                serializedData.data(),
                &data.assetID,
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                &data.specularStrength,
                sizeof(float)
            );
            pos += sizeof(float);

            memcpy(
                serializedData.data() + pos,
                &data.shininess,
                sizeof(float)
            );
            pos += sizeof(float);

            memcpy(
                serializedData.data() + pos,
                &data.textureOffset,
                sizeof(Vector2f)
            );
            pos += sizeof(Vector2f);

            memcpy(
                serializedData.data() + pos,
                &data.textureScale,
                sizeof(Vector2f)
            );
            pos += sizeof(Vector2f);

            memcpy(
                serializedData.data() + pos,
                &data.castShadows,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.receiveShadows,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.transparent,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.shadeless,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.persistent,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.blendmapTextureID,
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                &data.diffuseTextureIDs,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                serializedData.data() + pos,
                &data.specularTextureIDs,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                serializedData.data() + pos,
                &data.normalTextureIDs,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                serializedData.data() + pos,
                &data.name,
                metadata_name_size
            );
            pos += metadata_name_size;
            PLATYPUS_ASSERT(pos == material_metadata_serialized_size);

            return serializedData;
        }

        std::vector<char> serialize_model_metadata(ModelMetadata data)
        {
            std::vector<char> serializedData(model_metadata_serialized_size);
            memcpy(
                serializedData.data(),
                &data.assetID,
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                &data.instanced,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                serializedData.data() + pos,
                &data.persistent,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            for (size_t i = 0; i < metadata_model_max_meshes; ++i)
            {
                memcpy(
                    serializedData.data() + pos,
                    &(data.meshIDs[i]),
                    sizeof(ID_t)
                );
                pos += sizeof(ID_t);
            }

            memcpy(
                serializedData.data() + pos,
                &data.name,
                metadata_name_size
            );
            pos += metadata_name_size;

            memcpy(
                serializedData.data() + pos,
                &data.filepath,
                metadata_filepath_size
            );
            pos += metadata_filepath_size;
            PLATYPUS_ASSERT(pos == model_metadata_serialized_size);

            return serializedData;
        }

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == image_metadata_serialized_size);
            ImageMetadata metadata;

            memcpy(
                reinterpret_cast<void*>(&metadata.assetID),
                pData,
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.format),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ImageFormat)
            );
            pos += sizeof(ImageFormat);

            memcpy(
                reinterpret_cast<void*>(&metadata.persistent),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.name),
                reinterpret_cast<char*>(pData) + pos,
                metadata_name_size
            );
            pos += metadata_name_size;

            memcpy(
                reinterpret_cast<void*>(&metadata.filepath),
                reinterpret_cast<char*>(pData) + pos,
                metadata_filepath_size
            );
            return metadata;
        }

        TextureMetadata deserialize_texture_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == texture_metadata_serialized_size);
            TextureMetadata metadata;

            memcpy(
                reinterpret_cast<void*>(&metadata.assetID),
                reinterpret_cast<char*>(pData),
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.imageID),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.filterMode),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(TextureSamplerFilterMode)
            );
            pos += sizeof(TextureSamplerFilterMode);

            memcpy(
                reinterpret_cast<void*>(&metadata.addressMode),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(TextureSamplerAddressMode)
            );
            pos += sizeof(TextureSamplerAddressMode);

            memcpy(
                reinterpret_cast<void*>(&metadata.useMipmapping),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.persistent),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                metadata.name,
                reinterpret_cast<char*>(pData) + pos,
                metadata_name_size
            );
            pos += metadata_name_size;
            PLATYPUS_ASSERT(pos == texture_metadata_serialized_size);

            return metadata;
        }

        MaterialMetadata deserialize_material_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == material_metadata_serialized_size);
            MaterialMetadata metadata;
            const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;

            memcpy(
                &metadata.assetID,
                pData,
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                &metadata.specularStrength,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(float)
            );
            pos += sizeof(float);

            memcpy(
                &metadata.shininess,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(float)
            );
            pos += sizeof(float);

            memcpy(
                &metadata.textureOffset,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(Vector2f)
            );
            pos += sizeof(Vector2f);

            memcpy(
                &metadata.textureScale,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(Vector2f)
            );
            pos += sizeof(Vector2f);

            memcpy(
                &metadata.castShadows,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.receiveShadows,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.transparent,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.shadeless,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.persistent,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.blendmapTextureID,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                metadata.diffuseTextureIDs,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                metadata.specularTextureIDs,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                metadata.normalTextureIDs,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t) * texturesPerChannel
            );
            pos += sizeof(ID_t) * texturesPerChannel;

            memcpy(
                metadata.name,
                reinterpret_cast<char*>(pData) + pos,
                metadata_name_size
            );

            return metadata;
        }

        ModelMetadata deserialize_model_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == model_metadata_serialized_size);

            ModelMetadata metadata;
            memcpy(
                &metadata.assetID,
                pData,
                sizeof(ID_t)
            );
            size_t pos = sizeof(ID_t);

            memcpy(
                &metadata.instanced,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            memcpy(
                &metadata.persistent,
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint8_t)
            );
            pos += sizeof(uint8_t);

            for (size_t i = 0; i < metadata_model_max_meshes; ++i)
            {
                memcpy(
                    &(metadata.meshIDs[i]),
                    reinterpret_cast<char*>(pData) + pos,
                    sizeof(ID_t)
                );
                pos += sizeof(ID_t);
            }

            memcpy(
                &metadata.name,
                reinterpret_cast<char*>(pData) + pos,
                metadata_name_size
            );
            pos += metadata_name_size;

            memcpy(
                &metadata.filepath,
                reinterpret_cast<char*>(pData) + pos,
                metadata_filepath_size
            );
            pos += metadata_filepath_size;

            PLATYPUS_ASSERT(pos == model_metadata_serialized_size);
            return metadata;
        }

        /*
            Byte offsets:
                0 (4 bytes) = image count
                4 (4 bytes) = texture count
                8 (4 bytes) = material count
                12 (4 bytes) = model count
        */
        std::vector<char> serialize_assets(const std::vector<Asset*>& assets)
        {
            std::vector<char> fullMetadata(serialized_assets_header_size);

            std::vector<const Image*> imageAssets;
            std::vector<const Texture*> textureAssets;
            std::vector<const Material*> materialAssets;
            std::vector<const Model*> modelAssets;
            for (const Asset* pAsset : assets)
            {
                if (pAsset->getType() == AssetType::ASSET_TYPE_IMAGE)
                    imageAssets.push_back(reinterpret_cast<const Image*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_TEXTURE)
                    textureAssets.push_back(reinterpret_cast<const Texture*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_MATERIAL)
                    materialAssets.push_back(reinterpret_cast<const Material*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_MODEL)
                    modelAssets.push_back(reinterpret_cast<const Model*>(pAsset));
            }
            const size_t imageAssetCount = imageAssets.size();
            const size_t textureAssetCount = textureAssets.size();
            const size_t materialAssetCount = materialAssets.size();
            const size_t modelAssetCount = modelAssets.size();

            const uint32_t imageAssetCountU32 = static_cast<uint32_t>(imageAssetCount);
            const uint32_t textureAssetCountU32 = static_cast<uint32_t>(textureAssetCount);
            const uint32_t materialAssetCountU32 = static_cast<uint32_t>(materialAssetCount);
            const uint32_t modelAssetCountU32 = static_cast<uint32_t>(modelAssetCount);

            // Write header containing counts for each asset type
            memcpy(fullMetadata.data(), &imageAssetCountU32, sizeof(uint32_t));
            memcpy(
                fullMetadata.data() + sizeof(uint32_t),
                &textureAssetCountU32,
                sizeof(uint32_t)
            );
            memcpy(
                fullMetadata.data() + sizeof(uint32_t) * 2,
                &materialAssetCountU32,
                sizeof(uint32_t)
            );
            memcpy(
                fullMetadata.data() + sizeof(uint32_t) * 3,
                &modelAssetCountU32,
                sizeof(uint32_t)
            );

            for (size_t i = 0; i < imageAssetCount; ++i)
            {
                std::vector<char> imageMetadata = serialize_image_metadata(
                    get_image_metadata(imageAssets[i])
                );
                fullMetadata.insert(fullMetadata.end(), imageMetadata.begin(), imageMetadata.end());
            }

            for (size_t i = 0; i < textureAssetCount; ++i)
            {
                std::vector<char> textureMetadata = serialize_texture_metadata(
                    get_texture_metadata(textureAssets[i])
                );
                fullMetadata.insert(fullMetadata.end(), textureMetadata.begin(), textureMetadata.end());
            }

            for (size_t i = 0; i < materialAssetCount; ++i)
            {
                std::vector<char> materialMetadata = serialize_material_metadata(
                    get_material_metadata(materialAssets[i])
                );
                fullMetadata.insert(fullMetadata.end(), materialMetadata.begin(), materialMetadata.end());
            }

            for (size_t i = 0; i < modelAssetCount; ++i)
            {
                std::vector<char> modelMetadata = serialize_model_metadata(
                    get_model_metadata(modelAssets[i])
                );
                fullMetadata.insert(fullMetadata.end(), modelMetadata.begin(), modelMetadata.end());
            }
            return fullMetadata;
        }

        void deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures,
            std::vector<MaterialMetadata>& outMaterials,
            std::vector<ModelMetadata>& outModels
        )
        {
            PLATYPUS_ASSERT(dataSize >= serialized_assets_header_size);
            uint32_t imageCount = 0;
            uint32_t textureCount = 0;
            uint32_t materialCount = 0;
            uint32_t modelCount = 0;
            memcpy(&imageCount, pData, sizeof(uint32_t));
            memcpy(
                &textureCount,
                reinterpret_cast<char*>(pData) + sizeof(uint32_t),
                sizeof(uint32_t)
            );
            memcpy(
                &materialCount,
                reinterpret_cast<char*>(pData) + sizeof(uint32_t) * 2,
                sizeof(uint32_t)
            );
            memcpy(
                &modelCount,
                reinterpret_cast<char*>(pData) + sizeof(uint32_t) * 3,
                sizeof(uint32_t)
            );

            uint32_t textureSectionBegin = serialized_assets_header_size + imageCount * image_metadata_serialized_size;
            uint32_t textureSectionEnd = textureSectionBegin + textureCount * texture_metadata_serialized_size;

            uint32_t materialSectionBegin = textureSectionEnd;
            uint32_t materialSectionEnd = materialSectionBegin + materialCount * material_metadata_serialized_size;

            uint32_t modelSectionBegin = materialSectionEnd;
            uint32_t modelSectionEnd = modelSectionBegin + modelCount * model_metadata_serialized_size;

            size_t pos = serialized_assets_header_size;
            while (pos < dataSize)
            {
                if (pos < textureSectionBegin)
                {
                    PLATYPUS_ASSERT(pos + image_metadata_serialized_size <= dataSize);
                    ImageMetadata metadata = deserialize_image_metadata(
                        image_metadata_serialized_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outImages.push_back(metadata);
                    pos += image_metadata_serialized_size;
                }
                else if (pos >= textureSectionBegin && pos < textureSectionEnd)
                {
                    PLATYPUS_ASSERT(pos + texture_metadata_serialized_size <= dataSize);
                    TextureMetadata metadata = deserialize_texture_metadata(
                        texture_metadata_serialized_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outTextures.push_back(metadata);
                    pos += texture_metadata_serialized_size;
                }
                else if (pos >= materialSectionBegin && pos < materialSectionEnd)
                {
                    PLATYPUS_ASSERT(pos + material_metadata_serialized_size <= dataSize);
                    MaterialMetadata metadata = deserialize_material_metadata(
                        material_metadata_serialized_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outMaterials.push_back(metadata);
                    pos += material_metadata_serialized_size;
                }
                else if (pos >= modelSectionBegin && pos < modelSectionEnd)
                {
                    PLATYPUS_ASSERT(pos + model_metadata_serialized_size <= dataSize);
                    ModelMetadata metadata = deserialize_model_metadata(
                        model_metadata_serialized_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outModels.push_back(metadata);
                    pos += model_metadata_serialized_size;
                }
            }
        }

        void write_asset_metadata_file(
            const std::string& filepath,
            const std::vector<Asset*>& assets
        )
        {
            std::vector<char> fullMetadata = serialization::serialize_assets(assets);
            try
            {
                std::fstream file(filepath, std::ios::out | std::ios::binary);
                if (!file.is_open())
                {
                    Debug::log(
                        "Failed to create file to: " + filepath,
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                file.write(reinterpret_cast<const char*>(fullMetadata.data()), fullMetadata.size());
                file.close();
                Debug::log(
                    "File written to " + filepath + " with " + std::to_string(fullMetadata.size()) + " bytes",
                    Debug::MessageType::PLATYPUS_MESSAGE
                );
            }
            catch (const std::ifstream::failure& e)
            {
                Debug::log(
                    "Failed to write file to: " + filepath + "\n    " + e.what(),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            catch (const std::exception& e)
            {
                const std::string exceptionStr(e.what());
                Debug::log(
                    "Exception: " + exceptionStr,
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        void read_asset_metadata_file(
            const std::string& filepath,
            std::vector<serialization::ImageMetadata>& outImages,
            std::vector<serialization::TextureMetadata>& outTextures,
            std::vector<serialization::MaterialMetadata>& outMaterials,
            std::vector<serialization::ModelMetadata>& outModels
        )
        {
            try
            {
                std::ifstream file(filepath, std::ios::in | std::ios::ate | std::ios::binary);
                if (!file.is_open())
                {
                    Debug::log(
                        "Failed to open file from: " + filepath,
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                size_t totalSize = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<char> fullData(totalSize);
                file.read(fullData.data(), fullData.size());
                file.close();

                serialization::deserialize_assets(
                    totalSize,
                    fullData.data(),
                    outImages,
                    outTextures,
                    outMaterials,
                    outModels
                );
            }
            catch (const std::ifstream::failure& e)
            {
                Debug::log(
                    "Failed to read file from: " + filepath + "\n    " + e.what(),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
    }
}
