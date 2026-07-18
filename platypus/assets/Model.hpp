#pragma once

#include "Asset.hpp"
#include "Mesh.hpp"
#include <vector>

namespace platypus
{
    struct ModelMetadata
    {
        UUID_t assetID = NULL_UUID;
        uint8_t instanced = 0;
        uint8_t persistent = 0;
        UUID_t meshIDs[asset_metadata_model_max_meshes];
        char name[asset_metadata_name_size];
        char filepath[asset_metadata_filepath_size];
    };


    class Model : public Asset
    {
    private:
        std::string _filepath;
        // NOTE: Not sure if "assets inside assets" should rather be IDs to those instead of ptrs!
        //  -> ptrs will become issue when serializing
        std::vector<Mesh*> _meshes;

        // NOTE:
        // instanced property should be per mesh and NOT per model, this is just a quick hack fix
        // for serialization.
        // TODO: Fix everywhere!
        bool _instanced = false;

    public:
        // NOTE: Meshes ownership doesn't transfer here!
        Model(
            size_t uuidPool,
            const std::string& filepath,
            bool instanced,
            const std::vector<Mesh*>& meshes,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        ~Model();

        virtual void writeToMetadataBuffer(
            std::vector<char>& targetBuffer
        ) const override;

        static Model* create_from_metadata_buffer(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );

        static size_t get_serialized_metadata_size();

        inline const std::string& getFilepath() const { return _filepath; }
        inline bool isInstanced() const { return _instanced; }
        inline const std::vector<Mesh*>& getMeshes() const { return _meshes; }
    };
}
