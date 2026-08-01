#include "Asset.hpp"
#include "AssetManager.hpp"
#include "platypus/core/Debug.hpp"
#include <cstring>


namespace platypus
{
    std::string asset_type_to_string(AssetType type)
    {
        switch (type)
        {
            case AssetType::ASSET_TYPE_MESH: return "ASSET_TYPE_MESH";
            case AssetType::ASSET_TYPE_MODEL: return "ASSET_TYPE_MODEL";
            case AssetType::ASSET_TYPE_IMAGE: return "ASSET_TYPE_IMAGE";
            case AssetType::ASSET_TYPE_TEXTURE: return "ASSET_TYPE_TEXTURE";
            case AssetType::ASSET_TYPE_MATERIAL: return "ASSET_TYPE_MATERIAL";
            case AssetType::ASSET_TYPE_FONT: return "ASSET_TYPE_FONT";
            case AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA: return "ASSET_TYPE_SKELETAL_ANIMATION_DATA";
            default: return "ASSET_TYPE_NONE";
        }
    }

    Asset::Asset(
        size_t uuidPool,
        AssetType type,
        const std::string& name,
        UUID_t id,
        bool persistent
    ) :
        _uuidPool(uuidPool),
        _type(type),
        _name(name),
        _persistent(persistent)
    {
        if (id != NULL_UUID)
        {
            _id = id;
            UUID::occupy(id, _uuidPool);
        }
        else
        {
            _id = UUID::generate(_uuidPool);
        }
    }

    Asset::Asset(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    ) :
        _uuidPool(pAssetManager->getUUIDPool())
    {
        PLATYPUS_ASSERT((bufferPos  + asset_base_serialized_size) <= targetBuffer.size());

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&_type, pBuf, sizeof(AssetType));
        size_t pos = sizeof(AssetType);

        memcpy(&_id, pBuf + pos, sizeof(UUID_t));
        pos += sizeof(UUID_t);
        UUID::occupy(_id, _uuidPool);

        memcpy(&_customFlags, pBuf + pos, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        uint8_t persistent = 0;
        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);
        _persistent = static_cast<bool>(persistent);

        char name[asset_metadata_name_size];
        // *the serialized name data is empty at indices that aren't used so no need to set name bytes to 0 here
        memcpy(name, pBuf + pos, asset_metadata_name_size);
        _name = std::string(name);
        pos += asset_metadata_name_size;

        PLATYPUS_ASSERT(pos == asset_base_serialized_size);
    }

    Asset::~Asset()
    {
        if (_id == NULL_UUID)
        {
            Debug::log(
                "_id was NULL_UUID",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        UUID::erase(_id, _uuidPool);
    }

    /*
        Serialized format:
            AssetType _type
            UUID_t _id
            uint64_t _customFlags
            uint8_t _persistent
            char _name[asset_metadata_name_size];
    */
    void Asset::serializeBase(char* pData) const
    {
        if (_name.size() >= asset_metadata_name_size)
        {
            Debug::log(
                "Asset name: " + _name + " is too long for serialization! "
                "Max serialized asset name size is " + std::to_string(asset_metadata_name_size),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        memcpy(pData, &_type, sizeof(AssetType));
        size_t pos = sizeof(AssetType);

        memcpy(pData + pos, &_id, sizeof(UUID_t));
        pos += sizeof(UUID_t);

        memcpy(pData + pos, &_customFlags, sizeof(uint64_t));
        pos += sizeof(uint64_t);

        uint8_t persistent = static_cast<uint8_t>(_persistent);
        memcpy(pData + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        char nameData[asset_metadata_name_size];
        memset(nameData, 0, asset_metadata_name_size);
        memcpy(nameData, _name.data(), _name.size());
        memcpy(pData + pos, nameData, asset_metadata_name_size);
    }
}
