#pragma once

#include "Asset.h"
#include "Mesh.h"
#include "platypus/graphics/CommandBuffer.h"
#include <unordered_map>
#include <vector>


namespace platypus
{
    class AssetManager
    {
    private:
        CommandPool& _commandPoolRef;
        std::unordered_map<ID_t, Asset*> _assets;

    public:
        AssetManager(CommandPool& commandPool);
        ~AssetManager();
        void destroyAssets();

        Mesh* createMesh(
            const std::vector<float>& vertexData,
            const std::vector<uint32_t>& indexData
        );
        Mesh* getMesh(ID_t assetID) const;

    private:
        Asset* getAsset(ID_t assetID) const;
    };
}
