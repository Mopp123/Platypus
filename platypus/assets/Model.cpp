#include "Model.hpp"

namespace platypus
{
    // NOTE: Meshes ownership doesn't transfer here!
    Model::Model(const std::vector<Mesh*>& meshes) :
        Asset(AssetType::ASSET_TYPE_MODEL),
        _meshes(meshes)
    {
    }

    Model::~Model()
    {
    }
}
