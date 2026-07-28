#include "Model.hpp"
#include "AssetManager.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    // NOTE: Meshes ownership doesn't transfer here!
    Model::Model(
        size_t uuidPool,
        const std::string& filepath,
        bool instanced,
        const std::vector<Mesh*>& meshes,
        const std::string& name,
        UUID_t id,
        bool persistent
    ) :
        Asset(uuidPool, AssetType::ASSET_TYPE_MODEL, name, id, persistent),
        _filepath(filepath),
        _meshes(meshes),
        _instanced(instanced)
    {
    }

    Model::Model(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    ) :
        Asset(pAssetManager->getUUIDPool())
    {
        const size_t serializedSize = getSerializedSize();
        PLATYPUS_ASSERT((bufferPos  + serializedSize) <= targetBuffer.size());

        uint8_t instanced;
        uint8_t persistent;
        uint32_t meshCount;
        char name[asset_metadata_name_size];
        char filepath[asset_metadata_filepath_size];

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&_id, pBuf, sizeof(UUID_t));
        UUID::occupy(_id, _uuidPool);
        size_t pos = sizeof(UUID_t);

        memcpy(&_type, pBuf + pos, sizeof(AssetType));
        pos += sizeof(AssetType);

        memcpy(&_customFlags, pBuf + pos, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        memcpy(&instanced, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);
        _instanced = static_cast<bool>(instanced);

        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);
        _persistent = static_cast<bool>(persistent);

        memcpy(&meshCount, pBuf + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(&name, pBuf + pos, asset_metadata_name_size);
        pos += asset_metadata_name_size;
        _name = std::string(name);

        memcpy(&filepath, pBuf + pos, asset_metadata_filepath_size);
        pos += asset_metadata_filepath_size;
        _filepath = std::string(filepath);

        PLATYPUS_ASSERT(pos == serializedSize);

        std::vector<UUID_t> useMeshIDs(meshCount);
        _meshes.resize(meshCount);
        memcpy(useMeshIDs.data(), pBuf + pos, sizeof(UUID_t) * meshCount);
        pos += sizeof(UUID_t) * meshCount;

        pAssetManager->addExternalAsset(this);
        if (_persistent)
            pAssetManager->makePersistent(this);

        pAssetManager->addToDeserializationModelMeshUUIDQuery(_id, useMeshIDs);
    }

    Model::~Model()
    {
    }

    /*
        Serialized format:
            ID_t assetID
            AssetType type
            uint64_t customFlags
            uint8_t instanced
            uint8_t persistent
            uint32_t meshCount
            char name[metadata_name_size]
            char filepath[metadata_filepath_size]
            ID_t meshIDs[meshCount]
    */
    void Model::serialize(
        std::vector<char>& targetBuffer
    ) const
    {
        PLATYPUS_ASSERT(_name.size() <= asset_metadata_name_size);
        PLATYPUS_ASSERT(_filepath.size() <= asset_metadata_filepath_size);
        const size_t serializedSize = getSerializedSize();
        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + serializedSize);
        char* pBuf = targetBuffer.data() + prevSize;

        memcpy(pBuf, &_id, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(pBuf + pos, &_type, sizeof(AssetType));
        pos += sizeof(AssetType);

        memcpy(pBuf + pos, &_customFlags, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        const uint8_t instanced = static_cast<uint8_t>(_instanced);
        memcpy(pBuf + pos, &instanced, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t persistent = static_cast<const uint8_t>(_persistent);
        memcpy(pBuf + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint32_t meshCount = static_cast<const uint32_t>(_meshes.size());
        memcpy(pBuf + pos, &meshCount, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(pBuf + pos, _name.data(), _name.size());
        pos += asset_metadata_name_size;

        memcpy(pBuf + pos, _filepath.data(), _filepath.size());
        pos += asset_metadata_filepath_size;

        std::vector<UUID_t> meshIDs(_meshes.size());
        for (size_t i = 0; i < _meshes.size(); ++i)
            meshIDs[i] = _meshes[i]->getID();

        memcpy(pBuf + pos, meshIDs.data(), sizeof(UUID_t) * meshCount);
        pos += sizeof(UUID_t) * meshCount;

        PLATYPUS_ASSERT(pos == serializedSize);
    }

    size_t Model::getSerializedSize() const
    {
        return sizeof(UUID_t) +
            sizeof(AssetType) +
            asset_metadata_custom_flags_size +
            sizeof(uint8_t) * 2 +
            sizeof(uint32_t) +
            asset_metadata_name_size +
            asset_metadata_filepath_size +
            sizeof(UUID_t) * _meshes.size();
    }
}
