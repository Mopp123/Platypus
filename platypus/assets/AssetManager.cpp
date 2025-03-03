#include "AssetManager.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/utils/ModelLoading.h"


namespace platypus
{
    AssetManager::AssetManager(CommandPool& commandPool) :
        _commandPoolRef(commandPool)
    {
    }

    AssetManager::~AssetManager()
    {
        destroyAssets();
    }

    void AssetManager::destroyAssets()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
            delete it->second;

        _assets.clear();

        Debug::log("Assets destroyed");
    }

    Image* AssetManager::createImage(PE_ubyte* pData, int width, int height, int channels)
    {
        bool failure = false;
        if (!pData)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Provided data was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (width <= 0 || height <= 0)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Invalid image dimensions: " + std::to_string(width) + "x"+ std::to_string(height),
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (width <= 0 || height <= 0)
        {
            Debug::log(
                "@AssetManager::createImage "
                "Invalid channel count: " + std::to_string(channels),
                Debug::MessageType::PLATYPUS_ERROR
            );
            failure = true;
        }
        if (failure)
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        Image* pImage = new Image(pData, width, height, channels);
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Image* AssetManager::loadImage(const std::string& filepath)
    {
        Image* pImage = Image::load_image(filepath);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadImage "
                "Failed to load image from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Texture* AssetManager::createTexture(ID_t imageID, const TextureSampler& sampler)
    {
        Image* pImage = getImage(imageID);
        if (!pImage)
        {
            Debug::log(
                "AssetManager::createTexture "
                "Failed to find image with ID: " + std::to_string(imageID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        Texture* pTexture = new Texture(
            _commandPoolRef,
            pImage,
            sampler
        );
        _assets[pTexture->getID()] = pTexture;
        return pTexture;
    }

    Mesh* AssetManager::createMesh(
        const std::vector<float>& vertexData,
        const std::vector<uint32_t>& indexData
    )
    {
        Buffer* pVertexBuffer = new Buffer(
            _commandPoolRef,
            (void*)vertexData.data(),
            sizeof(float),
            vertexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );
        Buffer* pIndexBuffer = new Buffer(
            _commandPoolRef,
            (void*)indexData.data(),
            sizeof(uint32_t),
            indexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );
        Mesh* pMesh = new Mesh(pVertexBuffer, pIndexBuffer);
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Model* AssetManager::loadModel(const std::string& filepath)
    {
        std::vector<Buffer*> vertexBuffers;
        std::vector<Buffer*> indexBuffers;
        if (!load_gltf_model(_commandPoolRef, filepath, vertexBuffers, indexBuffers))
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        // Currently supporting only loading single mesh from file for model!
        // TODO: Multiple meshes and buffers per model!
        if (vertexBuffers.size() != indexBuffers.size())
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath + " "
                "Mismatch in meshes' index buffers and vertex buffers' sizes! "
                "It is currently required to have a single index buffer and vertex buffer"
                "for a single mesh!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> meshes(vertexBuffers.size());
        for (size_t i = 0; i < vertexBuffers.size(); ++i)
        {
            Mesh* pMesh = new Mesh(vertexBuffers[i], indexBuffers[i]);
            _assets[pMesh->getID()] = pMesh;
            meshes[i] = pMesh;
        }
        Model* pModel = new Model(meshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Image* AssetManager::getImage(ID_t assetID) const
    {
        Asset* pAsset = getAsset(assetID);
        if (!pAsset)
            return nullptr;

        if (pAsset->getType() != AssetType::ASSET_TYPE_IMAGE)
        {
            Debug::log(
                "@AssetManager::getImage "
                "Found asset with id: " + std::to_string(assetID) + " "
                "but it had invalid type: " + asset_type_to_string(pAsset->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return (Image*)pAsset;
    }

    Texture* AssetManager::getTexture(ID_t assetID) const
    {
        Asset* pAsset = getAsset(assetID);
        if (!pAsset)
            return nullptr;

        if (pAsset->getType() != AssetType::ASSET_TYPE_TEXTURE)
        {
            Debug::log(
                "@AssetManager::getTexture "
                "Found asset with id: " + std::to_string(assetID) + " "
                "but it had invalid type: " + asset_type_to_string(pAsset->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return (Texture*)pAsset;
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
