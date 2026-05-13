#include "Model.hpp"
#include "AssetManager.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    // NOTE: Meshes ownership doesn't transfer here!
    Model::Model(
        const std::string& filepath,
        bool instanced,
        const std::vector<Mesh*>& meshes,
        const std::string& name,
        ID_t id
    ) :
        Asset(AssetType::ASSET_TYPE_MODEL, name, id),
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

        memcpy(pBuf, &_id, sizeof(ID_t));
        size_t pos = sizeof(ID_t);

        const uint8_t instanced = static_cast<uint8_t>(_instanced);
        memcpy(pBuf + pos, &instanced, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        // TODO: Figure how to deal with this?
        uint8_t persistent = 0;
        memcpy(pBuf + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        ID_t meshIDs[asset_metadata_model_max_meshes];
        const size_t meshesSize = sizeof(ID_t) * asset_metadata_model_max_meshes;
        memset(meshIDs, NULL_ID, meshesSize);
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
        PLATYPUS_ASSERT((targetBuffer.size() + bufferPos  + get_serialized_metadata_size()) <= targetBuffer.size());

        ID_t id;
        uint8_t instanced;
        uint8_t persistent;
        ID_t meshIDs[asset_metadata_model_max_meshes];
        char name[asset_metadata_name_size];
        char filepath[asset_metadata_filepath_size];

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&id, pBuf, sizeof(ID_t));
        size_t pos = sizeof(ID_t);

        memcpy(&instanced, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const size_t meshesSize = sizeof(ID_t) * asset_metadata_model_max_meshes;
        memcpy(&meshIDs, pBuf + pos, meshesSize);
        pos += meshesSize;

        memcpy(&name, pBuf + pos, asset_metadata_name_size);
        pos += asset_metadata_name_size;

        memcpy(&filepath, pBuf + pos, asset_metadata_filepath_size);
        pos += asset_metadata_filepath_size;

        PLATYPUS_ASSERT(pos == get_serialized_metadata_size());

        std::vector<ID_t> useMeshIDs;
        for (size_t i = 0; i < asset_metadata_model_max_meshes; ++i)
        {
            if (meshIDs[i] != NULL_ID)
                useMeshIDs.push_back(meshIDs[i]);
        }

        Model* pModel = pAssetManager->loadModel(
            std::string(filepath),
            static_cast<bool>(instanced),
            std::string(name),
            id,
            useMeshIDs
        );

        if (persistent)
            pAssetManager->makePersistent(pModel);

        return pModel;
    }

    size_t Model::get_serialized_metadata_size()
    {
        return sizeof(ID_t) +
            sizeof(uint8_t) * 2 +
            sizeof(ID_t) * asset_metadata_model_max_meshes +
            asset_metadata_name_size +
            asset_metadata_filepath_size;
    }
}
