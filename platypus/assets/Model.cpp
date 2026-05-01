#include "Model.hpp"

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
}
