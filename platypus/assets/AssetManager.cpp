#include "AssetManager.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    AssetManager::~AssetManager()
    {}

    void AssetManager::destroyAssets()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
            delete it->second;

        _assets.clear();

        Debug::log("Assets destroyed");
    }

    Mesh* AssetManager::createMesh(Buffer* pVertexBuffer, Buffer* pIndexBuffer)
    {
        Mesh* pMesh = new Mesh(pVertexBuffer, pIndexBuffer);
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Mesh* AssetManager::getMesh(ID_t assetID) const
    {
        Asset* pAsset = getAsset(assetID);
        if (!pAsset)
            return nullptr;

        if (pAsset->getType() != AssetType::ASSET_TYPE_MESH)
        {
            Debug::log(
                "@AssetManager::getMesh "
                "Found asset with id: " + std::to_string(assetID) + " "
                "but it had invalid type: " + asset_type_to_string(pAsset->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return (Mesh*)pAsset;
    }

    Asset* AssetManager::getAsset(ID_t assetID) const
    {
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Failed to find asset with id: " + std::to_string(assetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return it->second;
    }
}
