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
        Asset(pAssetManager, targetBuffer, bufferPos)
    {
        const size_t baseAssetSerializedSize = getSerializedBaseSize();
        const size_t serializedModelBaseSize = baseAssetSerializedSize +
            sizeof(uint8_t) +
            sizeof(uint32_t);

        PLATYPUS_ASSERT((bufferPos + serializedModelBaseSize) <= targetBuffer.size());

        uint8_t instanced;
        uint32_t meshCount;
        uint32_t filepathSizeU32;

        const char* pBuf = targetBuffer.data() + bufferPos;
        size_t pos = baseAssetSerializedSize;

        memcpy(&instanced, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);
        _instanced = static_cast<bool>(instanced);

        memcpy(&meshCount, pBuf + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(&filepathSizeU32, pBuf + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);
        const size_t filepathSize = static_cast<const size_t>(filepathSizeU32);

        char* pFilepathData = new char[filepathSize];
        memcpy(pFilepathData, pBuf + pos, filepathSize);
        pos += filepathSize;
        _filepath = std::string(pFilepathData, filepathSize);
        delete[] pFilepathData;

        _meshes.resize(meshCount);
        memset(_meshes.data(), 0, sizeof(Mesh*) * meshCount);
        std::vector<UUID_t> useMeshIDs(meshCount);
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
            Asset serialized base data

            uint8_t instanced
            uint32_t meshCount
            uint32_t filepathSize
            char filepath[filepathSize]
            UUID_t meshIDs[meshCount]
    */
    void Model::serialize(
        std::vector<char>& targetBuffer
    ) const
    {
        const size_t serializedSize = getSerializedSize();
        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + serializedSize);
        char* pBuf = targetBuffer.data() + prevSize;

        serializeBase(pBuf);
        size_t pos = getSerializedBaseSize();

        const uint8_t instanced = static_cast<uint8_t>(_instanced);
        memcpy(pBuf + pos, &instanced, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint32_t meshCount = static_cast<const uint32_t>(_meshes.size());
        memcpy(pBuf + pos, &meshCount, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        const uint32_t filepathSize = static_cast<const uint32_t>(_filepath.size());
        memcpy(pBuf + pos, &filepathSize, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(pBuf + pos, _filepath.data(), _filepath.size());
        pos += _filepath.size();

        std::vector<UUID_t> meshIDs(_meshes.size());
        for (size_t i = 0; i < _meshes.size(); ++i)
            meshIDs[i] = _meshes[i]->getID();

        memcpy(pBuf + pos, meshIDs.data(), sizeof(UUID_t) * meshCount);
        pos += sizeof(UUID_t) * meshCount;

        PLATYPUS_ASSERT(pos == serializedSize);
    }

    size_t Model::getSerializedSize() const
    {
        return getSerializedBaseSize() +
            sizeof(uint8_t) + // instanced
            sizeof(uint32_t) + // meshCount
            sizeof(uint32_t) + // filepathSize
            _filepath.size() +
            sizeof(UUID_t) * _meshes.size();
    }
}
