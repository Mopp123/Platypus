#include "Serialization.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include <string>
#include <cstring>


namespace platypus
{
    namespace serialization
    {
        std::string image_metadata_to_string(const ImageMetadata& data)
        {
            std::string name(data.name);
            std::string filepath(data.filepath);
            return name + "\n"
                "   format = " + image_format_to_string(data.format) + "\n"
                "   filepath = " + filepath + "\n"
                "   persistent = " + std::to_string(data.persistent);
        }

        std::string texture_metadata_to_string(const TextureMetadata& data)
        {
            std::string name(data.name);
            return name + "\n"
                "   filterMode = " + texture_sampler_filter_mode_to_string(data.filterMode) + "\n"
                "   addressMode = " + texture_sampler_address_mode_to_string(data.addressMode) + "\n"
                "   useMipmapping = " + std::to_string(data.useMipmapping) + "\n"
                "   imageID = " + std::to_string(data.imageID) + "\n"
                "   persistent = " + std::to_string(data.persistent);
        }

        std::string material_metadata_to_string(const MaterialMetadata& data)
        {
            std::string name(data.name);

            std::string str = name + "\n"
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

        ImageMetadata get_image_metadata(const Image* pImage)
        {
            const std::string& assetName = pImage->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            const std::string& filepath = pImage->getFilepath();
            PLATYPUS_ASSERT(filepath.size() < metadata_filepath_size);

            ImageMetadata data;
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
            PLATYPUS_ASSERT(pTexture->getImage());
            PLATYPUS_ASSERT(pTexture->getImage()->getID() != NULL_ID);

            const TextureSampler* pSampler = pTexture->getTextureSampler();
            TextureMetadata data;
            data.filterMode = pSampler->getFilterMode();
            data.addressMode = pSampler->getAddressMode();
            data.useMipmapping = static_cast<uint8_t>(pSampler->isMipmapped());
            data.persistent = static_cast<uint8_t>(pTexture->isPersistent());
            data.imageID = pTexture->getImage()->getID();
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

        std::vector<char> serialize_image_metadata(ImageMetadata data)
        {
            std::vector<char> serializedData(image_metadata_serialized_size);
            memcpy(
                serializedData.data(),
                reinterpret_cast<void*>(&data.format),
                sizeof(ImageFormat)
            );
            size_t pos = sizeof(ImageFormat);

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
            memcpy(
                serializedData.data(),
                reinterpret_cast<void*>(&data.filterMode),
                sizeof(TextureSamplerFilterMode)
            );
            size_t pos = sizeof(TextureSamplerFilterMode);

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
                reinterpret_cast<void*>(&data.imageID),
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                serializedData.data() + pos,
                data.name,
                metadata_name_size
            );

            return serializedData;
        }

        std::vector<char> serialize_material_metadata(MaterialMetadata data)
        {
            std::vector<char> serializedData(material_metadata_serialized_size);
            const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;

            memcpy(
                serializedData.data(),
                &data.specularStrength,
                sizeof(float)
            );
            size_t pos = sizeof(float);

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

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == image_metadata_serialized_size);
            ImageMetadata metadata;

            memcpy(
                reinterpret_cast<void*>(&metadata.format),
                pData,
                sizeof(ImageFormat)
            );
            size_t pos = sizeof(ImageFormat);

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
                reinterpret_cast<void*>(&metadata.filterMode),
                pData,
                sizeof(TextureSamplerFilterMode)
            );
            size_t pos = sizeof(TextureSamplerFilterMode);

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
                reinterpret_cast<void*>(&metadata.imageID),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(ID_t)
            );
            pos += sizeof(ID_t);

            memcpy(
                metadata.name,
                reinterpret_cast<char*>(pData) + pos,
                metadata_name_size
            );

            return metadata;
        }

        MaterialMetadata deserialize_material_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == material_metadata_serialized_size);
            MaterialMetadata metadata;
            const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;

            memcpy(
                &metadata.specularStrength,
                pData,
                sizeof(float)
            );
            size_t pos = sizeof(float);

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

        /*
            Byte offsets:
                0 (4 bytes) = image count
                4 (4 bytes) = texture count
                8 (4 bytes) = material count
        */
        std::vector<char> serialize_assets(const std::vector<Asset*>& assets)
        {
            std::vector<char> fullMetadata(serialized_assets_header_size);

            std::vector<const Image*> imageAssets;
            std::vector<const Texture*> textureAssets;
            std::vector<const Material*> materialAssets;
            for (const Asset* pAsset : assets)
            {
                if (pAsset->getType() == AssetType::ASSET_TYPE_IMAGE)
                    imageAssets.push_back(reinterpret_cast<const Image*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_TEXTURE)
                    textureAssets.push_back(reinterpret_cast<const Texture*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_MATERIAL)
                    materialAssets.push_back(reinterpret_cast<const Material*>(pAsset));
            }
            const size_t imageAssetCount = imageAssets.size();
            const size_t textureAssetCount = textureAssets.size();
            const size_t materialAssetCount = materialAssets.size();

            const uint32_t imageAssetCountU32 = static_cast<uint32_t>(imageAssetCount);
            const uint32_t textureAssetCountU32 = static_cast<uint32_t>(textureAssetCount);
            const uint32_t materialAssetCountU32 = static_cast<uint32_t>(materialAssetCount);

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
            return fullMetadata;
        }

        void deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures,
            std::vector<MaterialMetadata>& outMaterials
        )
        {
            PLATYPUS_ASSERT(dataSize >= serialized_assets_header_size);
            uint32_t imageCount = 0;
            uint32_t textureCount = 0;
            uint32_t materialCount = 0;
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

            uint32_t textureSectionBegin = serialized_assets_header_size + imageCount * image_metadata_serialized_size;
            uint32_t textureSectionEnd = textureSectionBegin + textureCount * texture_metadata_serialized_size;

            uint32_t materialSectionBegin = textureSectionEnd;
            uint32_t materialSectionEnd = materialSectionBegin + materialCount * material_metadata_serialized_size;

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
            }
        }
    }
}
