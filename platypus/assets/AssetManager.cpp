#include "AssetManager.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/assets/SkeletalAnimationData.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/modelLoading/ModelLoading.h"
#include "platypus/Common.h"


namespace platypus
{
    AssetManager::AssetManager()
    {
        // Create black and white default textures
        PE_ubyte whitePixels[4] = { 255, 255, 255, 255 };
        Image* pWhiteImage = createImage(whitePixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        _persistentAssets[pWhiteImage->getID()] = pWhiteImage;

        PE_ubyte blackPixels[4] = { 0, 0, 0, 255 };
        Image* pBlackImage = createImage(blackPixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        _persistentAssets[pBlackImage->getID()] = pBlackImage;

        PE_ubyte zeroPixels[4] = { 0, 0, 0, 0 };
        Image* pZeroImage = createImage(zeroPixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        _persistentAssets[pZeroImage->getID()] = pZeroImage;

        TextureSampler defaultTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            1,
            0
        );
        _pWhiteTexture = createTexture(
            pWhiteImage->getID(),
            defaultTextureSampler
        );
        _pBlackTexture = createTexture(
            pBlackImage->getID(),
            defaultTextureSampler
        );
        _pZeroTexture = createTexture(
            pZeroImage->getID(),
            defaultTextureSampler
        );
        _persistentAssets[_pWhiteTexture->getID()] = _pWhiteTexture;
        _persistentAssets[_pBlackTexture->getID()] = _pBlackTexture;
        _persistentAssets[_pZeroTexture->getID()] = _pZeroTexture;
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

    Image* AssetManager::createImage(PE_ubyte* pData, int width, int height, int channels, ImageFormat format)
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
        Image* pImage = new Image(pData, width, height, channels, format);
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Image* AssetManager::loadImage(const std::string& filepath, ImageFormat format)
    {
        Image* pImage = Image::load_image(filepath, format);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadImage "
                "Failed to load image from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
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
            pImage,
            sampler,
            textureAtlasRows
        );
        _assets[pTexture->getID()] = pTexture;
        return pTexture;
    }

    Texture* AssetManager::loadTexture(
        const std::string& filepath,
        ImageFormat format,
        const TextureSampler& sampler,
        uint32_t textureAtlasRows
    )
    {
        Image* pImage = loadImage(filepath, format);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadTexture "
                "Failed to load texture",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        return createTexture(pImage->getID(), sampler, textureAtlasRows);
    }

    Material* AssetManager::createMaterial(
        MaterialType type,
        ID_t blendmapTextureID,
        std::vector<ID_t> diffuseTextureIDs,
        std::vector<ID_t> specularTextureIDs,
        std::vector<ID_t> normalTextureIDs,
        float specularStrength,
        float shininess,
        bool shadeless
    )
    {
        // At least a single diffuse texture is required.
        // If not provided or invalid -> use some default texture
        if (diffuseTextureIDs.empty())
            diffuseTextureIDs.push_back(_pBlackTexture->getID());

        // If textures were provided -> validate they exist
        if (blendmapTextureID)
        {
            if (!validateAsset("createMaterial (blendmap texture validation)", blendmapTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }
        for (ID_t diffuseTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (diffuse texture validation)", diffuseTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }
        for (ID_t specularTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (specular texture validation)", specularTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }
        for (ID_t normalTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (normal texture validation)", normalTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }

        Material* pMaterial = new Material(
            type,
            blendmapTextureID,
            diffuseTextureIDs.data(),
            specularTextureIDs.data(),
            normalTextureIDs.data(),
            diffuseTextureIDs.size(),
            specularTextureIDs.size(),
            normalTextureIDs.size(),
            specularStrength,
            shininess,
            shadeless
        );
        _assets[pMaterial->getID()] = pMaterial;
        return pMaterial;
    }

    Mesh* AssetManager::createMesh(
        const VertexBufferLayout& vertexBufferLayout,
        const std::vector<float>& vertexData,
        const std::vector<uint32_t>& indexData
    )
    {
        Buffer* pVertexBuffer = new Buffer(
            (void*)vertexData.data(),
            sizeof(float),
            vertexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Buffer* pIndexBuffer = new Buffer(
            (void*)indexData.data(),
            sizeof(uint32_t),
            indexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Mesh* pMesh = new Mesh(
            vertexBufferLayout,
            pVertexBuffer,
            pIndexBuffer,
            Matrix4f(1.0f)
        );
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Model* AssetManager::loadModel(const std::string& filepath)
    {
        std::vector<MeshData> loadedMeshes;
        std::vector<KeyframeAnimationData> loadedAnimations;
        if (!load_gltf_model(filepath, loadedMeshes, loadedAnimations))
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> createdMeshes;
        for (const MeshData& meshData : loadedMeshes)
        {
            Buffer* pVertexBuffer = new Buffer(
                (void*)meshData.vertexBufferData.rawData.data(),
                meshData.vertexBufferData.elementSize,
                meshData.vertexBufferData.length,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );
            MeshBufferData useIndexBuffer = meshData.indexBufferData[0];
            Buffer* pIndexBuffer = new Buffer(
                (void*)useIndexBuffer.rawData.data(),
                useIndexBuffer.elementSize,
                useIndexBuffer.length,
                BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );

            Mesh* pMesh = new Mesh(
                meshData.vertexBufferLayout,
                pVertexBuffer,
                pIndexBuffer,
                meshData.transformationMatrix
            );
            _assets[pMesh->getID()] = pMesh;
            createdMeshes.push_back(pMesh);
        }
        Model* pModel = new Model(createdMeshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Model* AssetManager::loadModel(
        const std::string& filepath,
        std::vector<KeyframeAnimationData>& outAnimations
    )
    {
        std::vector<MeshData> loadedMeshes;
        if (!load_gltf_model(filepath, loadedMeshes, outAnimations))
        {
            Debug::log(
                "@AssetManager::loadModel "
                "Failed to load model using filepath: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> createdMeshes;
        for (const MeshData& meshData : loadedMeshes)
        {
            Buffer* pVertexBuffer = new Buffer(
                (void*)meshData.vertexBufferData.rawData.data(),
                meshData.vertexBufferData.elementSize,
                meshData.vertexBufferData.length,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );
            MeshBufferData useIndexBuffer = meshData.indexBufferData[0];
            Buffer* pIndexBuffer = new Buffer(
                (void*)useIndexBuffer.rawData.data(),
                useIndexBuffer.elementSize,
                useIndexBuffer.length,
                BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );

            Mesh* pMesh = new Mesh(
                meshData.vertexBufferLayout,
                pVertexBuffer,
                pIndexBuffer,
                meshData.transformationMatrix,
                meshData.bindPose
            );
            _assets[pMesh->getID()] = pMesh;
            createdMeshes.push_back(pMesh);
        }
        Model* pModel = new Model(createdMeshes);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Mesh* AssetManager::createTerrainMesh(
        float tileSize,
        const std::vector<float>& heightmapData,
        bool dynamic,
        bool generateTangents
    )
    {
        Mesh* pTerrainMesh = Mesh::generate_terrain(tileSize, heightmapData, dynamic, generateTangents);
        _assets[pTerrainMesh->getID()] = pTerrainMesh;
        return pTerrainMesh;
    }

    SkeletalAnimationData* AssetManager::createSkeletalAnimation(
        const KeyframeAnimationData& keyframes
    )
    {
        SkeletalAnimationData* pAnimationData = new SkeletalAnimationData(
            keyframes
        );
        _assets[pAnimationData->getID()] = pAnimationData;
        return pAnimationData;
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

    bool AssetManager::assetExists(ID_t assetID, AssetType type) const
    {
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it != _assets.end())
        {
            if (it->second->getType() == type)
                return true;
        }
        return false;
    }

    Asset* AssetManager::getAsset(ID_t assetID) const
    {
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Failed to find asset with ID: " + std::to_string(assetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        return it->second;
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
            return nullptr;
        }
        if (it->second->getType() != type)
        {
            Debug::log(
                "@AssetManager::getAsset "
                "Requested asset of type " + asset_type_to_string(type) + " "
                "Found asset with id: " + std::to_string(assetID) + " " +
                "but the asset's type was " + asset_type_to_string(it->second->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        return it->second;
    }

    std::vector<Asset*> AssetManager::getAssets(AssetType type) const
    {
        std::vector<Asset*> foundAssets;
        for (const auto& asset : _assets)
        {
            if (asset.second->getType() == type)
                foundAssets.push_back(asset.second);
        }
        return foundAssets;
    }

    void AssetManager::addExternalPersistentAsset(Asset* pAsset)
    {
        _assets[pAsset->getID()] = pAsset;
        _persistentAssets[pAsset->getID()] = pAsset;
    }

    void AssetManager::destroyExternalPersistentAsset(Asset* pAsset)
    {
        ID_t assetID = pAsset->getID();
        delete _assets[assetID];
        _assets.erase(assetID);
        _persistentAssets.erase(assetID);
    }

    bool AssetManager::validateAsset(
        const char* callLocation,
        ID_t assetID,
        AssetType requiredType
    )
    {
        const std::string callLocationStr(callLocation);
        const std::string locationStr(PLATYPUS_CURRENT_FUNC_NAME);
        std::unordered_map<ID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it == _assets.end())
        {
            Debug::log(
                "@AssetManager::" + locationStr + "(" + locationStr + ") "
                "Failed to find asset with ID: " + std::to_string(assetID) + " "
                "with type: " + asset_type_to_string(requiredType),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        if (_assets[assetID]->getType() != requiredType)
        {
            Debug::log(
                "@AssetManager::" + locationStr + "(" + locationStr + ") "
                "Invalid asset type: " + asset_type_to_string(_assets[assetID]->getType()) + " "
                "for asset ID: " + std::to_string(assetID) + " "
                "required type was: " + asset_type_to_string(requiredType),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }
}
