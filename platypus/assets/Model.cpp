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

    Model::~Model()
    {
    }

    /*
        Serialized format (in order):
            ID_t assetID = NULL_ID;
            uint64_t customFlags;
            uint8_t instanced = 0;
            uint8_t persistent = 0;
            ID_t meshIDs[metadata_model_max_meshes];
            char name[metadata_name_size];
            char filepath[metadata_filepath_size];
    */
    void Model::writeToMetadataBuffer(
        std::vector<char>& targetBuffer
    ) const
    {
        PLATYPUS_ASSERT(_name.size() <= asset_metadata_name_size);
        PLATYPUS_ASSERT(_filepath.size() <= asset_metadata_filepath_size);

        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + get_serialized_metadata_size());
        char* pBuf = targetBuffer.data() + prevSize;

        memcpy(pBuf, &_id, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(pBuf + pos, &_customFlags, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        const uint8_t instanced = static_cast<uint8_t>(_instanced);
        memcpy(pBuf + pos, &instanced, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t persistent = static_cast<const uint8_t>(_persistent);
        memcpy(pBuf + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        UUID_t meshIDs[asset_metadata_model_max_meshes];
        const size_t meshesSize = sizeof(UUID_t) * asset_metadata_model_max_meshes;
        memset(meshIDs, NULL_UUID, meshesSize);
        for (size_t i = 0; i < _meshes.size(); ++i)
            meshIDs[i] = _meshes[i]->getID();

        memcpy(pBuf + pos, meshIDs, meshesSize);
        pos += meshesSize;

        memcpy(pBuf + pos, _name.data(), _name.size());
        pos += asset_metadata_name_size;

        memcpy(pBuf + pos, _filepath.data(), _filepath.size());
        pos += asset_metadata_filepath_size;

        PLATYPUS_ASSERT(pos == get_serialized_metadata_size());
    }

    Model* Model::create_from_metadata_buffer(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    )
    {
        PLATYPUS_ASSERT((bufferPos  + get_serialized_metadata_size()) <= targetBuffer.size());

        UUID_t id;
        uint64_t customFlags;
        uint8_t instanced;
        uint8_t persistent;
        UUID_t meshIDs[asset_metadata_model_max_meshes];
        char name[asset_metadata_name_size];
        char filepath[asset_metadata_filepath_size];

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&id, pBuf, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(&customFlags, pBuf + pos, asset_metadata_custom_flags_size);
        pos += asset_metadata_custom_flags_size;

        memcpy(&instanced, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const size_t meshesSize = sizeof(UUID_t) * asset_metadata_model_max_meshes;
        memcpy(&meshIDs, pBuf + pos, meshesSize);
        pos += meshesSize;

        memcpy(&name, pBuf + pos, asset_metadata_name_size);
        pos += asset_metadata_name_size;

        memcpy(&filepath, pBuf + pos, asset_metadata_filepath_size);
        pos += asset_metadata_filepath_size;

        PLATYPUS_ASSERT(pos == get_serialized_metadata_size());

        std::vector<UUID_t> useMeshIDs;
        for (size_t i = 0; i < asset_metadata_model_max_meshes; ++i)
        {
            if (meshIDs[i] != NULL_UUID)
                useMeshIDs.push_back(meshIDs[i]);
        }

        Model* pModel = pAssetManager->loadModel(
            std::string(filepath),
            static_cast<bool>(instanced),
            std::string(name),
            id,
            useMeshIDs
        );
        pModel->_customFlags = customFlags;

        if (persistent)
        {
            const std::vector<Mesh*>& meshes = pModel->getMeshes();
            for (Mesh* pMesh : meshes)
                pAssetManager->makePersistent(pMesh);

            pAssetManager->makePersistent(pModel);
        }

        return pModel;
    }

    size_t Model::get_serialized_metadata_size()
    {
        return sizeof(UUID_t) +
            asset_metadata_custom_flags_size +
            sizeof(uint8_t) * 2 +
            sizeof(UUID_t) * asset_metadata_model_max_meshes +
            asset_metadata_name_size +
            asset_metadata_filepath_size;
    }
}
