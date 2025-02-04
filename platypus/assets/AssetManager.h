#pragma once

#include "Asset.h"
#include "Mesh.h"
#include <unordered_map>


namespace platypus
{
    class AssetManager
    {
    private:
        std::unordered_map<ID_t, Asset*> _assets;

    public:
        ~AssetManager();
        void destroyAssets();

        Mesh* createMesh(Buffer* pVertexBuffer, Buffer* pIndexBuffer);
        Mesh* getMesh(ID_t assetID) const;

    private:
        Asset* getAsset(ID_t assetID) const;
    };
}
