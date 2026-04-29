#include "Serialization.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include <string>
#include <cstring>


namespace platypus
{
    namespace serialization
    {
        ImageMetadata get_image_metadata(const Image* pImage)
        {
            const std::string& assetName = pImage->getName();
            PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
            const std::string& filepath = pImage->getFilepath();
            PLATYPUS_ASSERT(filepath.size() < metadata_filepath_size);

            ImageMetadata data;
            data.format = pImage->getFormat();
            data.persistent = static_cast<uint32_t>(pImage->isPersistent());
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
            data.useMipmapping = static_cast<uint32_t>(pSampler->isMipmapped());
            data.persistent = static_cast<uint32_t>(pTexture->isPersistent());
            data.imageID = pTexture->getImage()->getID();
            memset(data.name, 0, metadata_name_size);
            memcpy(data.name, assetName.data(), assetName.size());
            return data;
        }

        std::vector<char> serialize_image_metadata(ImageMetadata data)
        {
            std::vector<char> serializedData(image_metadata_file_size);
            memcpy(
                serializedData.data(),
                reinterpret_cast<void*>(&data.format),
                sizeof(ImageFormat)
            );
            size_t pos = sizeof(ImageFormat);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.persistent),
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

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
            std::vector<char> serializedData(texture_metadata_file_size);
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
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<void*>(&data.persistent),
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

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

        ImageMetadata deserialize_image_metadata(size_t dataSize, void* pData)
        {
            PLATYPUS_ASSERT(dataSize == image_metadata_file_size);
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
                sizeof(ImageFormat)
            );
            pos += sizeof(uint32_t);

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
            PLATYPUS_ASSERT(dataSize == texture_metadata_file_size);
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
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

            memcpy(
                reinterpret_cast<void*>(&metadata.persistent),
                reinterpret_cast<char*>(pData) + pos,
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

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


        /*
            Byte offsets:
                0 (4 bytes) = image count
                4 (4 bytes) = texture count
        */
        std::vector<char> serialize_assets(const std::vector<Asset*>& assets)
        {
            std::vector<char> fullMetadata(serialized_assets_header_size);

            std::vector<const Image*> imageAssets;
            std::vector<const Texture*> textureAssets;
            for (const Asset* pAsset : assets)
            {
                if (pAsset->getType() == AssetType::ASSET_TYPE_IMAGE)
                    imageAssets.push_back(reinterpret_cast<const Image*>(pAsset));
                else if (pAsset->getType() == AssetType::ASSET_TYPE_TEXTURE)
                    textureAssets.push_back(reinterpret_cast<const Texture*>(pAsset));
            }
            const size_t imageAssetCount = imageAssets.size();
            const uint32_t imageAssetCountU32 = static_cast<uint32_t>(imageAssetCount);
            const size_t textureAssetCount = textureAssets.size();
            const uint32_t textureAssetCountU32 = static_cast<uint32_t>(textureAssetCount);

            // Write header containing counts for each asset type
            memcpy(fullMetadata.data(), &imageAssetCountU32, sizeof(uint32_t));
            memcpy(fullMetadata.data() + sizeof(uint32_t), &textureAssetCountU32, sizeof(uint32_t));

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
            return fullMetadata;
        }

        void deserialize_assets(
            size_t dataSize,
            void* pData,
            std::vector<ImageMetadata>& outImages,
            std::vector<TextureMetadata>& outTextures
        )
        {
            PLATYPUS_ASSERT(dataSize >= serialized_assets_header_size);
            uint32_t imageCount = 0;
            uint32_t textureCount = 0;
            memcpy(&imageCount, pData, sizeof(uint32_t));
            memcpy(&textureCount, reinterpret_cast<char*>(pData) + sizeof(uint32_t), sizeof(uint32_t));

            uint32_t textureSectionBegin = serialized_assets_header_size + imageCount * image_metadata_file_size;
            uint32_t textureSectionEnd = textureSectionBegin + textureCount * texture_metadata_file_size;

            size_t pos = sizeof(uint32_t) * 2;
            while (pos < dataSize)
            {
                if (pos < textureSectionBegin)
                {
                    PLATYPUS_ASSERT(pos + image_metadata_file_size <= dataSize);
                    ImageMetadata metadata = deserialize_image_metadata(
                        image_metadata_file_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outImages.push_back(metadata);
                    pos += image_metadata_file_size;
                }
                else if (pos >= textureSectionBegin && pos < textureSectionEnd)
                {
                    PLATYPUS_ASSERT(pos + texture_metadata_file_size <= dataSize);
                    TextureMetadata metadata = deserialize_texture_metadata(
                        texture_metadata_file_size,
                        reinterpret_cast<char*>(pData) + pos
                    );
                    outTextures.push_back(metadata);
                    pos += texture_metadata_file_size;
                }
            }
        }
    }
}
