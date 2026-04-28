#include "Serialization.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include <string>
#include <cstring>


namespace platypus
{
    namespace serialization
    {
        ImageMetadata get_image_metadata(const Image* pImage, const std::string& assetName)
        {
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

        //TextureMetadata get_texture_metadata(const Texture* pTexture, const std::string& assetName)
        //{
        //    PLATYPUS_ASSERT(assetName.size() < metadata_name_size);
        //    PLATYPUS_ASSERT(pTexture->getImage()->getID() != NULL_ID);
        //}

        std::vector<char> serialize_image_metadata(ImageMetadata data)
        {
            std::vector<char> serializedData(image_metadata_file_size);
            memcpy(
                serializedData.data(),
                reinterpret_cast<const char*>(&data.format),
                sizeof(ImageFormat)
            );
            size_t pos = sizeof(ImageFormat);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<const char*>(&data.persistent),
                sizeof(uint32_t)
            );
            pos += sizeof(uint32_t);

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<const char*>(&data.name),
                metadata_name_size
            );
            pos += metadata_name_size;

            memcpy(
                serializedData.data() + pos,
                reinterpret_cast<const char*>(&data.filepath),
                metadata_filepath_size
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
    }
}
