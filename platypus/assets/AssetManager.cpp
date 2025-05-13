#include "AssetManager.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/utils/ModelLoading.h"
#include "platypus/Common.h"


namespace platypus
{
    AssetManager::AssetManager(CommandPool& commandPool) :
        _commandPoolRef(commandPool)
    {
        // Create white default texture
        PE_ubyte whitePixels[4] = { 255, 255, 255, 255 };
        Image* pWhiteImage = createImage(whitePixels, 1, 1, 4);
        _persistentAssets[pWhiteImage->getID()] = pWhiteImage;

        TextureSampler whiteTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            1,
            0
        );
        _pWhiteTexture = createTexture(pWhiteImage->getID(), whiteTextureSampler);
        _persistentAssets[_pWhiteTexture->getID()] = _pWhiteTexture;
    }

    AssetManager::~AssetManager()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
            delete it->second;

        _assets.clear();
        _persistentAssets.clear();
    }

    void AssetManager::destroyAssets()
    {
        std::unordered_map<ID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            if (_persistentAssets.find(it->first) == _persistentAssets.end())
                delete it->second;
        }
        _assets.clear();

        // Re add persistent assets to assets
        std::unordered_map<ID_t, Asset*>::iterator persistentIt;
        for (persistentIt = _persistentAssets.begin(); persistentIt != _persistentAssets.end(); ++persistentIt)
            _assets[persistentIt->first] = persistentIt->second;

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

    Texture* AssetManager::createTexture(
        ID_t imageID,
        const TextureSampler& sampler,
        uint32_t textureAtlasRows
    )
    {
        Image* pImage = (Image*)getAsset(imageID, AssetType::ASSET_TYPE_IMAGE);
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
            sampler,
            textureAtlasRows
        );
        Debug::log("___TEST___CREATED TEXTURE WITH: " + std::to_string(textureAtlasRows) + " ROWS");
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
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Buffer* pIndexBuffer = new Buffer(
            _commandPoolRef,
            (void*)indexData.data(),
            sizeof(uint32_t),
            indexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Mesh* pMesh = new Mesh(pVertexBuffer, pIndexBuffer, Matrix4f(1.0f));
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Model* AssetManager::loadModel(const std::string& filepath)
    {
        std::vector<std::vector<Buffer*>> indexBuffers;
        std::vector<Buffer*> vertexBuffers;
        std::vector<Matrix4f> transformationMatrices;
        if (!load_gltf_model(_commandPoolRef, filepath, indexBuffers, vertexBuffers, transformationMatrices))
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
            Mesh* pMesh = new Mesh(vertexBuffers[i], indexBuffers[i][0], transformationMatrices[i]);
            _assets[pMesh->getID()] = pMesh;
            meshes[i] = pMesh;
        }
        Model* pModel = new Model(meshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Font* AssetManager::loadFont(const std::string& filepath, unsigned int pixelSize)
    {
        Font* pFont = new Font;
        if (!pFont->load(filepath, pixelSize))
        {
            Debug::log(
                "@AssetManager::loadFont "
                "Failed to load font from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            delete pFont;
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        _assets[pFont->getID()] = pFont;
        return pFont;
    }

    Asset* AssetManager::getAsset(ID_t assetID, AssetType type) const
    {
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Failed to find asset of type: " + asset_type_to_string(type) + " "
                "with id: " + std::to_string(assetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return it->second;
    }
}
