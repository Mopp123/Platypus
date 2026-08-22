#include "AssetManager.hpp"
#include "platypus/graphics/Buffers.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/utils/modelLoading/ModelLoading.hpp"
#include "platypus/Common.h"


namespace platypus
{
    AssetManager::AssetManager()
    {
        _uuidPool = UUID::occupy_pool();
    }

    AssetManager::~AssetManager()
    {
        destroyDefaultAssets();

        for (TextureSampler* pSampler : _textureSamplers)
            delete pSampler;

        if (!_assets.empty())
        {
            Debug::log(
                "All assets haven't yet been destroyed!"
                " Remaining assets: " + std::to_string(_assets.size()) + " "
                "from which " + std::to_string(_persistentAssets.size()) + " were persistent.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void AssetManager::createDefaultAssets()
    {
        // Create black and white default textures
        PE_ubyte whitePixels[4] = { 255, 255, 255, 255 };
        _pWhiteImage = createImage(whitePixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        _pWhiteImage->setSerializable(false);

        PE_ubyte blackPixels[4] = { 0, 0, 0, 255 };
        _pBlackImage = createImage(blackPixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        _pBlackImage->setSerializable(false);

        PE_ubyte zeroPixels[4] = { 0, 0, 0, 0 };
        _pZeroImage = createImage(zeroPixels, 1, 1, 4, ImageFormat::R8G8B8A8_UNORM);
        _pZeroImage->setSerializable(false);

        const std::string defaultErrorImageLocation = "assets/textures/Error.png";
        _pErrorImage = loadImage(
            defaultErrorImageLocation,
            ImageFormat::R8G8B8A8_SRGB
        );
        if (!_pErrorImage)
        {
            Debug::log(
                "Failed to find default error image from: " + defaultErrorImageLocation + " "
                "Generating default(red) error image.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_WARNING
            );
            PE_ubyte redPixels[4] = { 255, 0, 0, 255 };
            _pErrorImage = createImage(redPixels, 1, 1, 4, ImageFormat::R8G8B8A8_SRGB);
        }
        _pErrorImage->setSerializable(false);

        _persistentAssets[_pWhiteImage->getID()] = _pWhiteImage;
        _persistentAssets[_pBlackImage->getID()] = _pBlackImage;
        _persistentAssets[_pZeroImage->getID()] = _pZeroImage;
        _persistentAssets[_pErrorImage->getID()] = _pErrorImage;

        _defaultAssets.insert(_pWhiteImage->getID());
        _defaultAssets.insert(_pBlackImage->getID());
        _defaultAssets.insert(_pZeroImage->getID());
        _defaultAssets.insert(_pErrorImage->getID());

        const TextureSampler* pDefaultTextureSampler = createTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            true
        );

        _pWhiteTexture = createTexture(
            _pWhiteImage->getID(),
            pDefaultTextureSampler
        );
        _pWhiteTexture->setSerializable(false);

        _pBlackTexture = createTexture(
            _pBlackImage->getID(),
            pDefaultTextureSampler
        );
        _pBlackTexture->setSerializable(false);

        _pZeroTexture = createTexture(
            _pZeroImage->getID(),
            pDefaultTextureSampler
        );
        _pZeroTexture->setSerializable(false);

        _pErrorTexture = createTexture(
            _pErrorImage->getID(),
            pDefaultTextureSampler
        );
        _pErrorTexture->setSerializable(false);

        _persistentAssets[_pWhiteTexture->getID()] = _pWhiteTexture;
        _persistentAssets[_pBlackTexture->getID()] = _pBlackTexture;
        _persistentAssets[_pZeroTexture->getID()] = _pZeroTexture;
        _persistentAssets[_pErrorTexture->getID()] = _pErrorTexture;

        _defaultAssets.insert(_pWhiteTexture->getID());
        _defaultAssets.insert(_pBlackTexture->getID());
        _defaultAssets.insert(_pZeroTexture->getID());
        _defaultAssets.insert(_pErrorTexture->getID());

        // TODO: Make some dumb shader which isn't using any lighting at all
        // for "dumb" shadeless materials!
        _pErrorMaterial = createMaterial(
            NULL_UUID,
            { _pErrorTexture->getID() },
            { _pWhiteTexture->getID() },
            { },
            0,
            0,
            { 0, 0 },
            { 1, 1 },
            false,
            false,
            false,
            true // shadeless
        );
        makePersistent(_pErrorMaterial);
        _defaultAssets.insert(_pErrorMaterial->getID());

        _pErrorModel = loadModel(
            "assets/models/Error.glb",
            false, // instanced
            ""
        );
        PLATYPUS_ASSERT(_pErrorModel);
        if (_pErrorModel)
        {
            makePersistent(_pErrorModel);
            _defaultAssets.insert(_pErrorModel->getID());
            PLATYPUS_ASSERT(!_pErrorModel->getMeshes().empty());

            _pErrorMesh = _pErrorModel->getMeshes()[0];
            _defaultAssets.insert(_pErrorMesh->getID());
        }
    }

    void AssetManager::destroyAssets()
    {
        std::unordered_map<UUID_t, Asset*>::iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            if (_persistentAssets.find(it->first) == _persistentAssets.end())
            {
                delete it->second;
                //it->second = nullptr;
                _assets[it->first] = nullptr;
            }
        }
        _assets.clear();

        // Re add persistent assets to assets
        std::unordered_map<UUID_t, Asset*>::iterator persistentIt;
        for (persistentIt = _persistentAssets.begin(); persistentIt != _persistentAssets.end(); ++persistentIt)
            _assets[persistentIt->first] = persistentIt->second;

        Debug::log("Assets destroyed");
    }

    void AssetManager::destroyAsset(UUID_t assetID)
    {
        if (_assets.find(assetID) == _assets.end())
        {
            Debug::log(
                "Asset id: " + std::to_string(assetID) + " not found",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        delete _assets[assetID];
        _assets.erase(assetID);

        if (_persistentAssets.find(assetID) != _persistentAssets.end())
            _persistentAssets.erase(assetID);

        if (_defaultAssets.find(assetID) != _defaultAssets.end())
            _defaultAssets.erase(assetID);
    }

    void AssetManager::destroyAsset(const std::string& assetName)
    {
        if (assetName.empty())
            return;

        UUID_t assetID = NULL_UUID;
        std::unordered_map<UUID_t, Asset*>::const_iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            if (it->second->getName() == assetName)
            {
                assetID = it->first;
                break;
            }
        }
        destroyAsset(assetID);
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
        Image* pImage = new Image(_uuidPool, pData, width, height, channels, format);
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Image* AssetManager::loadImage(
        const std::string& filepath,
        ImageFormat format,
        const std::string& name,
        UUID_t id
    )
    {
        if (!name.empty() && !nameAvailable(name))
        {
            _errors.push_back("Name " + name + " not available");
            return nullptr;
        }

        int width = -1;
        int height = -1;
        int channels = -1;
        PE_ubyte* pPixels = nullptr;
        if (!Image::read_image_pixels(
                filepath,
                &width,
                &height,
                &channels,
                &pPixels
            ))
        {
            std::string error = "Failed to load image from: " + filepath;
            Debug::log(
                error,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            _errors.push_back(error);
            return nullptr;
        }
        Image* pImage = new Image(
            _uuidPool,
            pPixels,
            width,
            height,
            channels,
            format,
            name,
            id
        );
        pImage->setFilepath(filepath);
        delete[] pPixels;
        // Old way of loading images below...
        /*
        Image* pImage = Image::load_image(_uuidPool, filepath, format, name, id);
        if (!pImage)
        {
            Debug::log(
                "@AssetManager::loadImage "
                "Failed to load image from: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        */
        _assets[pImage->getID()] = pImage;
        return pImage;
    }

    Texture* AssetManager::createTexture(
        UUID_t imageID,
        const TextureSampler* pSampler,
        const std::string& name,
        UUID_t id
    )
    {
        if (!name.empty() && !nameAvailable(name))
        {
            _errors.push_back("Name " + name + " not available");
            return nullptr;
        }

        Image* pImage = (Image*)getAsset(imageID, AssetType::ASSET_TYPE_IMAGE);
        if (!pImage)
        {
            const std::string error = "Failed to find image with ID: " + std::to_string(imageID);
            Debug::log(
                error,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            _errors.push_back(error);
            return nullptr;
        }

        const ImageFormat imageFormat = pImage->getFormat();
        const int imageColorChannels = pImage->getChannels();
        if (!is_image_format_valid(imageFormat, imageColorChannels))
        {
            const std::string error = "Invalid target format: " + image_format_to_string(imageFormat) + " "
                "for image with " + std::to_string(imageColorChannels) + " channels";

            Debug::log(
                error,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            _errors.push_back(error);
            return nullptr;
        }

        Texture* pTexture = new Texture(
            _uuidPool,
            pImage,
            pSampler,
            name,
            id
        );
        _assets[pTexture->getID()] = pTexture;
        return pTexture;
    }

    Texture* AssetManager::createTexture(
        UUID_t imageID,
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool useMipmapping,
        const std::string& name,
        UUID_t id
    )
    {
        const TextureSampler* pSampler = getTextureSampler(filterMode, addressMode, useMipmapping);
        if (!pSampler)
            pSampler = createTextureSampler(filterMode, addressMode, useMipmapping);

        return createTexture(imageID, pSampler, name, id);
    }

    Texture* AssetManager::loadTexture(
        const std::string& filepath,
        ImageFormat format,
        const TextureSampler* pSampler,
        const std::string& name,
        UUID_t id
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
        return createTexture(pImage->getID(), pSampler, name, id);
    }

    Material* AssetManager::createMaterial(
        UUID_t blendmapTextureID,
        std::vector<UUID_t> diffuseTextureIDs,
        std::vector<UUID_t> specularTextureIDs,
        std::vector<UUID_t> normalTextureIDs,
        float specularStrength,
        float shininess,
        const Vector2f& textureOffset,
        const Vector2f& textureScale,
        bool castShadows,
        bool receiveShadows,
        bool transparent,
        bool shadeless,
        const std::string& name,
        UUID_t id,
        const std::string& customVertexShaderFilename,
        const std::string& customFragmentShaderFilename
    )
    {
        if (!name.empty() && !nameAvailable(name))
        {
            _errors.push_back("Name " + name + " not available");
            return nullptr;
        }

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
        for (UUID_t diffuseTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (diffuse texture validation)", diffuseTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }
        for (UUID_t specularTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (specular texture validation)", specularTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }
        for (UUID_t normalTextureID : diffuseTextureIDs)
        {
            if (!validateAsset("createMaterial (normal texture validation)", normalTextureID, AssetType::ASSET_TYPE_TEXTURE))
                return nullptr;
        }

        Material* pMaterial = new Material(
            _uuidPool,
            blendmapTextureID,
            diffuseTextureIDs.data(),
            specularTextureIDs.data(),
            normalTextureIDs.data(),
            diffuseTextureIDs.size(),
            specularTextureIDs.size(),
            normalTextureIDs.size(),
            specularStrength,
            shininess,
            textureOffset,
            textureScale,
            castShadows,
            receiveShadows,
            transparent,
            shadeless,
            name,
            id,
            false, // is persistent?
            customVertexShaderFilename,
            customFragmentShaderFilename
        );
        _assets[pMaterial->getID()] = pMaterial;
        return pMaterial;
    }

    Material* AssetManager::createMaterial(
        UUID_t blendmapTextureID,
        std::vector<UUID_t> diffuseTextureIDs,
        std::vector<UUID_t> specularTextureIDs,
        std::vector<UUID_t> normalTextureIDs,
        float specularStrength,
        float shininess
    )
    {
        return createMaterial(
            blendmapTextureID,
            diffuseTextureIDs,
            specularTextureIDs,
            normalTextureIDs,
            specularStrength,
            shininess,
            { 0, 0 },
            { 1, 1 },
            false, // cast shadow
            false, // receive shadow
            false, // transparent
            false, // shadeless
            "",    // name
            NULL_UUID, // id
            "",
            ""
        );
    }

    Mesh* AssetManager::createMesh(
        const VertexBufferLayout& vertexBufferLayout,
        const std::vector<float>& vertexData,
        const void* pIndicesData,
        size_t indicesElementSize,
        size_t indicesLength,
        uint32_t meshPropertyFlags,
        bool storeHostsideBuffersOnDeserialization
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
            pIndicesData,
            indicesElementSize,
            indicesLength,
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        Mesh* pMesh = new Mesh(
            _uuidPool,
            meshPropertyFlags,
            vertexBufferLayout,
            pVertexBuffer,
            pIndexBuffer,
            Matrix4f(1.0f),
            { }, // bind pose
            { } // animations
        );
        pMesh->storeHostsideBuffersOnDeserialization(storeHostsideBuffersOnDeserialization);
        _assets[pMesh->getID()] = pMesh;
        return pMesh;
    }

    Mesh* AssetManager::createTerrainMesh(
        float tileSize,
        const std::vector<float>& heightmapData,
        bool dynamic,
        bool generateTangents,
        bool storeHostSideBuffers
    )
    {
        Mesh* pTerrainMesh = Mesh::generate_terrain(
            _uuidPool,
            tileSize,
            heightmapData,
            dynamic,
            generateTangents,
            storeHostSideBuffers
        );
        pTerrainMesh->storeHostsideBuffersOnDeserialization(storeHostSideBuffers);
        _assets[pTerrainMesh->getID()] = pTerrainMesh;
        return pTerrainMesh;
    }

    Model* AssetManager::loadModel(
        const std::string& filepath,
        bool instanced,
        const std::string& name,
        UUID_t modelID,
        std::vector<UUID_t> meshIDs,
        bool storeBuffersHostSide
    )
    {
        if (!name.empty() && !nameAvailable(name))
        {
            _errors.push_back("Name " + name + " not available");
            return nullptr;
        }

        std::vector<MeshData> loadedMeshes;
        std::vector<SkeletonData> loadedSkeletons;
        // NOTE: This is pretty fragile with animations and skinned meshes atm!
        if (!load_gltf_model(filepath, loadedMeshes, loadedSkeletons))
        {
            Debug::log(
                "Failed to load model using filepath: " + filepath,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            //PLATYPUS_ASSERT(false);
            _errors.push_back("Failed to load model with filepath: " + filepath);
            return nullptr;
        }

        if (!loadedSkeletons.empty() && instanced)
        {
            Debug::log(
                "Instancing not supported for skinned/animated meshes",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        std::vector<Mesh*> createdMeshes;
        for (size_t i = 0; i < loadedMeshes.size(); ++i)
        {
            const MeshData& meshData = loadedMeshes[i];
            const std::string meshName = name + "." + meshData.name;
            Buffer* pVertexBuffer = new Buffer(
                (void*)meshData.vertexBufferData.rawData.data(),
                meshData.vertexBufferData.elementSize,
                meshData.vertexBufferData.length,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                storeBuffersHostSide
            );
            MeshBufferData useIndexBuffer = meshData.indexBufferData[0];
            Buffer* pIndexBuffer = new Buffer(
                (void*)useIndexBuffer.rawData.data(),
                useIndexBuffer.elementSize,
                useIndexBuffer.length,
                BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                storeBuffersHostSide
            );

            uint32_t meshPropertyFlags = 0;
            if (!loadedSkeletons.empty())
                meshPropertyFlags |= static_cast<uint32_t>(MeshPropertyFlagBits::TYPE_SKINNED);
            else
                meshPropertyFlags |= static_cast<uint32_t>(MeshPropertyFlagBits::TYPE_STATIC);

            if (instanced)
                meshPropertyFlags |= static_cast<uint32_t>(MeshPropertyFlagBits::INSTANCED);

            // Add TANGENTS to mesh property flags if found from VertexBufferLayout
            for (const VertexBufferElement& vertexBufferElement : meshData.vertexBufferLayout.getElements())
            {
                if (vertexBufferElement.getAttribType() == VertexAttributeType::TANGENT)
                {
                    meshPropertyFlags |= static_cast<uint32_t>(MeshPropertyFlagBits::HAS_TANGENTS);
                    break;
                }
            }

            if (instanced && (meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::TYPE_SKINNED)))
            {
                Debug::log(
                    "Mesh had animations but was also marked to use instancing. "
                    "Currently instanced animated meshes aren't supported!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            UUID_t meshID = NULL_UUID;
            if (i < meshIDs.size())
                meshID = meshIDs[i];

            UUID_t skeletonAssetID = NULL_UUID;
            if (i < loadedSkeletons.size())
            {
                const SkeletonData& skeletonData = loadedSkeletons[i];

                const std::string& skeletonName = skeletonData.name;
                const size_t animationCount = skeletonData.animations.size();
                std::vector<UUID_t> animationIDs(animationCount);
                for (size_t animationIndex = 0; animationIndex < animationCount; ++animationIndex)
                {
                    SkeletalAnimationData* pSkeletalAnimationAsset = createSkeletalAnimation(
                        skeletonData.animations[animationIndex]
                    );
                    animationIDs[animationIndex] = pSkeletalAnimationAsset->getID();
                }

                const Pose& skeletonPose = skeletonData.bindPose;
                Skeleton* pSkeleton = createSkeleton(
                    skeletonPose.joints,
                    skeletonPose.jointChildMapping,
                    animationIDs,
                    skeletonName
                );
                skeletonAssetID = pSkeleton->getID();
            }

            Mesh* pMesh = new Mesh(
                _uuidPool,
                meshPropertyFlags,
                meshData.vertexBufferLayout,
                pVertexBuffer,
                pIndexBuffer,
                meshData.transformationMatrix,
                skeletonAssetID,
                meshName,
                meshID
            );
            _assets[pMesh->getID()] = pMesh;
            createdMeshes.push_back(pMesh);
            pMesh->storeHostsideBuffersOnDeserialization(storeBuffersHostSide);
        }
        Model* pModel = new Model(_uuidPool, filepath, instanced, createdMeshes, name, modelID);
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Model* AssetManager::createModel(
        const std::string& filepath,
        const std::vector<Mesh*>& meshes,
        bool instanced,
        const std::string& name
    )
    {
        if (!name.empty() && !nameAvailable(name))
        {
            _errors.push_back("Name " + name + " not available");
            return nullptr;
        }

        Model* pModel = new Model(
            _uuidPool,
            filepath,
            instanced,
            meshes,
            name
        );
        _assets[pModel->getID()] = pModel;
        return pModel;
    }

    Skeleton* AssetManager::createSkeleton(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        const std::vector<UUID_t>& animationIDs,
        const std::string& name
    )
    {
        Skeleton* pSkeleton = new Skeleton(
            _uuidPool,
            joints,
            jointChildMapping,
            animationIDs,
            name
        );
        _assets[pSkeleton->getID()] = pSkeleton;
        return pSkeleton;
    }

    SkeletalAnimationData* AssetManager::createSkeletalAnimation(
        const KeyframeAnimationData& animationData
    )
    {
        if (assetExists(animationData.name))
        {
            const std::string errStr = "Animation asset with name: '" + animationData.name + "' already exists!";
            _errors.push_back(errStr);
            Debug::log(
                errStr,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }

        SkeletalAnimationData* pAnimationData = new SkeletalAnimationData(
            _uuidPool,
            animationData
        );
        _assets[pAnimationData->getID()] = pAnimationData;
        return pAnimationData;
    }

    Font* AssetManager::loadFont(const std::string& filepath, unsigned int pixelSize)
    {
        Font* pFont = new Font(_uuidPool);
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

    const TextureSampler* AssetManager::createTextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool useMipmapping
    )
    {
        if (getTextureSampler(filterMode, addressMode, useMipmapping))
        {
            Debug::log(
                "Texture sampler with given properties already exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        TextureSampler* pSampler = new TextureSampler(
            filterMode,
            addressMode,
            useMipmapping,
            0
        );
        //_textureSamplers.push_back({
        //    filterMode,
        //    addressMode,
        //    useMipmapping,
        //    0
        //});
        //return &_textureSamplers[_textureSamplers.size() - 1];
        _textureSamplers.push_back(pSampler);
        return pSampler;
    }

    const TextureSampler* AssetManager::getTextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool useMipmapping
    ) const
    {
        for (size_t i = 0; i < _textureSamplers.size(); ++i)
        {
            const TextureSampler* pSampler = _textureSamplers[i];
            if (pSampler->getFilterMode() == filterMode &&
                pSampler->getAddressMode() == addressMode &&
                pSampler->isMipmapped() == useMipmapping)
            {
                return _textureSamplers[i];
            }
        }
        return nullptr;
    }

    const TextureSampler* AssetManager::getOrCreateTextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool useMipmapping
    )
    {
        const TextureSampler* pSampler = getTextureSampler(
            filterMode,
            addressMode,
            useMipmapping
        );
        if (!pSampler)
        {
            pSampler = createTextureSampler(
                filterMode,
                addressMode,
                useMipmapping
            );
        }
        return pSampler;
    }

    bool AssetManager::assetExists(UUID_t assetID) const
    {
        std::unordered_map<UUID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it != _assets.end())
            return true;

        return false;
    }

    bool AssetManager::assetExists(UUID_t assetID, AssetType type) const
    {
        std::unordered_map<UUID_t, Asset*>::const_iterator it = _assets.find(assetID);
        if (it != _assets.end())
        {
            if (it->second->getType() == type)
                return true;
        }
        return false;
    }

    bool AssetManager::assetExists(const std::string& assetName) const
    {
        if (assetName.empty())
            return false;

        std::unordered_map<UUID_t, Asset*>::const_iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            if (it->second->getName() == assetName)
                return true;
        }
        return false;
    }

    Asset* AssetManager::getAsset(UUID_t assetID) const
    {
        std::unordered_map<UUID_t, Asset*>::const_iterator it = _assets.find(assetID);
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

    Asset* AssetManager::getAsset(UUID_t assetID, AssetType type) const
    {
        std::unordered_map<UUID_t, Asset*>::const_iterator it = _assets.find(assetID);
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

    // NOTE: This is pretty fucking inefficient!
    Asset* AssetManager::getAsset(const std::string& assetName) const
    {
        if (assetName.empty())
            return nullptr;

        std::unordered_map<UUID_t, Asset*>::const_iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            Asset* pAsset = it->second;
            if (pAsset->getName() == assetName)
                return pAsset;
        }
        return nullptr;
    }

    std::vector<Asset*> AssetManager::getAssets(
        bool excludeInternalDefaults,
        bool excludeNonSerializable
    ) const
    {
        std::vector<Asset*> assets;
        for (const auto& asset : _assets)
        {
            Asset* pAsset = asset.second;
            if (excludeInternalDefaults)
            {
                if (_defaultAssets.find(pAsset->getID()) != _defaultAssets.end())
                    continue;
            }
            if (excludeNonSerializable)
            {
                if (!pAsset->isSerializable())
                    continue;
            }
            assets.push_back(asset.second);
        }
        return assets;
    }

    std::vector<Asset*> AssetManager::getAssets(
        AssetType type,
        bool excludeInternalDefaults,
        bool excludeNonSerializable
    ) const
    {
        std::vector<Asset*> foundAssets;
        for (const auto& asset : _assets)
        {
            Asset* pAsset = asset.second;
            if (!pAsset)
                continue;

            if (pAsset->getType() == type)
            {
                if (excludeInternalDefaults)
                {
                    if (_defaultAssets.find(pAsset->getID()) != _defaultAssets.end())
                        continue;
                }
                if (excludeNonSerializable)
                {
                    if (!pAsset->isSerializable())
                        continue;
                }
                foundAssets.push_back(asset.second);
            }
        }
        return foundAssets;
    }

    UUID_t AssetManager::getMeshModel(UUID_t meshID) const
    {
        std::unordered_map<UUID_t, Asset*>::const_iterator it;
        for (it = _assets.begin(); it != _assets.end(); ++it)
        {
            const Asset* pAsset = it->second;
            if (pAsset->getType() == AssetType::ASSET_TYPE_MODEL)
            {
                const Model* pModel = dynamic_cast<const Model*>(pAsset);
                for (const Mesh* pMesh : pModel->getMeshes())
                {
                    if (pMesh->getID() == meshID)
                        return pModel->getID();
                }
            }
        }
        return NULL_UUID;
    }

    void AssetManager::makePersistent(Asset* pAsset)
    {
        pAsset->setPersistent(true);
        _persistentAssets[pAsset->getID()] = pAsset;

        // WARNING!
        // TODO: Consider having some "sub asset container" for the base asset
        // instead of having ptrs to assets inside the assets themselves!!!
        if (pAsset->getType() == AssetType::ASSET_TYPE_MODEL)
        {
            Model* pModel = reinterpret_cast<Model*>(pAsset);
            for (Mesh* pMesh : pModel->getMeshes())
            {
                if (pMesh)
                    makePersistent(pMesh);
            }
        }
    }

    void AssetManager::addExternalDefaultAsset(Asset* pAsset)
    {
        UUID_t assetID = pAsset->getID();
        if (_defaultAssets.find(assetID) != _defaultAssets.end())
        {
            Debug::log(
                "Asset UUID: " + std::to_string(assetID) + " "
                "was already added as external default!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        _assets[assetID] = pAsset;
        makePersistent(pAsset);
        _defaultAssets.insert(assetID);
    }

    void AssetManager::addExternalAsset(Asset* pAsset)
    {
        const UUID_t assetID = pAsset->getID();
        std::unordered_map<UUID_t, Asset*>::iterator it = _assets.find(assetID);
        if (it != _assets.end())
        {
            Debug::log(
                "Asset with UUID: " + std::to_string(assetID) + " and name: " + pAsset->getName() + " "
                "already exists in AssetManager",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _assets[assetID] = pAsset;
    }

    bool AssetManager::isPersistent(UUID_t assetID) const
    {
        return _persistentAssets.find(assetID) != _persistentAssets.end();
    }

    void AssetManager::addToDeserializationModelMeshUUIDQuery(
        UUID_t modelUUID,
        const std::vector<UUID_t>& meshUUIDs
    )
    {
        std::unordered_map<UUID_t, std::vector<UUID_t>>::const_iterator it = _modelAssetsToFinalize.find(modelUUID);
        if (it != _modelAssetsToFinalize.end())
        {
            Debug::log(
                "Model with UUID: " + std::to_string(modelUUID) + " was already added!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _modelAssetsToFinalize[modelUUID] = meshUUIDs;
    }

    void AssetManager::addToDeserializationTextureImageUUIDQuery(UUID_t textureUUID, UUID_t imageUUID)
    {
        std::unordered_map<UUID_t, UUID_t>::const_iterator it = _textureAssetsToFinalize.find(textureUUID);
        if (it != _textureAssetsToFinalize.end())
        {
            Debug::log(
                "Texture with UUID: " + std::to_string(textureUUID) + " was already added!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _textureAssetsToFinalize[textureUUID] = imageUUID;
    }

    void AssetManager::addToDeserializationMaterialUUIDQuery(UUID_t materialUUID)
    {
        if (_materialAssetsToFinalize.find(materialUUID) != _materialAssetsToFinalize.end())
        {
            Debug::log(
                "Material with UUID: " + std::to_string(materialUUID) + " was already added!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _materialAssetsToFinalize.insert(materialUUID);
    }

    /*
        Serialized format:
            uint32_t assetCount
    */
    std::vector<char> AssetManager::serialize(
        const std::vector<Asset*>& assets
    )
    {
        std::vector<char> buffer(sizeof(uint32_t));
        uint32_t assetCount = static_cast<uint32_t>(assets.size());
        memcpy(buffer.data(), &assetCount, sizeof(uint32_t));

        for (const Asset* pAsset : assets)
            pAsset->serialize(buffer);

        return buffer;
    }


    size_t AssetManager::deserializeHeader(
        const std::vector<char>& serializedData,
        size_t* pAssetCount
    ) const
    {
        uint32_t assetCount;
        memcpy(&assetCount, serializedData.data(), sizeof(uint32_t));
        if (pAssetCount)
            *pAssetCount = static_cast<size_t>(assetCount);

        return sizeof(uint32_t);
    }

    Asset* AssetManager::deserialize(
        const std::vector<char>& serializedData,
        size_t bufferReadPos,
        size_t& bufferReadEndPos
    )
    {
        const size_t serializedAssetsHeaderSize = sizeof(uint32_t);
        PLATYPUS_ASSERT(bufferReadPos >= serializedAssetsHeaderSize);
        const size_t bufferSize = serializedData.size();
        PLATYPUS_ASSERT(bufferReadPos <= bufferSize);

        const char* pBuf = serializedData.data() + bufferReadPos;

        AssetType type;
        memcpy(&type, pBuf, sizeof(AssetType));
        Debug::log("___TEST___attempting to read asset type: " + asset_type_to_string(type));

        Asset* pAsset = nullptr;
        switch (type)
        {
            case AssetType::ASSET_TYPE_MESH:     pAsset = new Mesh(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_MODEL:    pAsset = new Model(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_IMAGE:    pAsset = new Image(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_TEXTURE:  pAsset = new Texture(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_MATERIAL: pAsset = new Material(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_SKELETON: pAsset = new Skeleton(this, serializedData, bufferReadPos); break;
            case AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA: pAsset = new SkeletalAnimationData(this, serializedData, bufferReadPos); break;
            default: {
                Debug::log(
                    "Invalid asset type: " + asset_type_to_string(type) + " for deserialization",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        bufferReadEndPos = bufferReadPos + pAsset->getSerializedSize();
        return pAsset;
    }

    std::unordered_map<std::string, Asset*> AssetManager::deserialize(
        const std::vector<char>& serializedData,
        size_t& lastReadPos
    )
    {
        std::unordered_map<std::string, Asset*> outAssets;

        const size_t serializedAssetsHeaderSize = sizeof(uint32_t);
        const size_t bufferSize = serializedData.size();
        PLATYPUS_ASSERT(bufferSize >= serializedAssetsHeaderSize);
        uint32_t assetCount = 0;
        memcpy(&assetCount, serializedData.data(), sizeof(uint32_t));

        size_t offset = sizeof(uint32_t);
        for (uint32_t i = 0; i < assetCount; ++i)
        {
            Asset* pAsset = deserialize(
                serializedData,
                offset,
                offset
            );
            outAssets[pAsset->getName()] = pAsset;
        }

        return outAssets;
    }

    void AssetManager::finalizeDeserialization()
    {
        std::unordered_map<UUID_t, std::vector<UUID_t>>::iterator modelIt;
        for (modelIt = _modelAssetsToFinalize.begin(); modelIt != _modelAssetsToFinalize.end(); ++modelIt)
        {
            Asset* pModelAsset = getAsset(modelIt->first, AssetType::ASSET_TYPE_MODEL);
            PLATYPUS_ASSERT(pModelAsset);
            Model* pModel = reinterpret_cast<Model*>(pModelAsset);
            const std::vector<UUID_t> meshUUIDs = modelIt->second;
            PLATYPUS_ASSERT(pModel->_meshes.size() == meshUUIDs.size());
            for (size_t i = 0; i < pModel->_meshes.size(); ++i)
            {
                UUID_t meshUUID = meshUUIDs[i];
                Asset* pMeshAsset = getAsset(meshUUID, AssetType::ASSET_TYPE_MESH);
                PLATYPUS_ASSERT(pMeshAsset);
                Mesh* pMesh = reinterpret_cast<Mesh*>(pMeshAsset);
                // TODO: Maybe have just the mesh UUIDs in the Model asset
                //  -> Makes shit a lot easier?
                pModel->_meshes[i] = pMesh;
            }
        }
        _modelAssetsToFinalize.clear();

        std::unordered_map<UUID_t, UUID_t>::iterator textureIt;
        for (textureIt = _textureAssetsToFinalize.begin(); textureIt != _textureAssetsToFinalize.end(); ++textureIt)
        {
            Asset* pTextureAsset = getAsset(textureIt->first, AssetType::ASSET_TYPE_TEXTURE);
            PLATYPUS_ASSERT(pTextureAsset);
            Texture* pTexture = reinterpret_cast<Texture*>(pTextureAsset);

            Asset* pImageAsset = getAsset(textureIt->second, AssetType::ASSET_TYPE_IMAGE);
            PLATYPUS_ASSERT(pImageAsset);
            Image* pImage = reinterpret_cast<Image*>(pImageAsset);

            pTexture->create(pImage);
        }
        _textureAssetsToFinalize.clear();

        for (UUID_t materialUUID : _materialAssetsToFinalize)
        {
            Asset* pMaterialAsset = getAsset(materialUUID, AssetType::ASSET_TYPE_MATERIAL);
            PLATYPUS_ASSERT(pMaterialAsset);
            Material* pMaterial = reinterpret_cast<Material*>(pMaterialAsset);
            pMaterial->createShaderResources();
        }
        _materialAssetsToFinalize.clear();
    }

    std::vector<std::string> AssetManager::popErrors()
    {
        std::vector<std::string> errors = _errors;
        _errors.clear();
        return errors;
    }

    std::string AssetManager::popError()
    {
        if (_errors.empty())
            return "";

        std::string error = _errors.back();
        _errors.pop_back();
        return error;
    }

    // TODO: Figure out better way to deal with asset names!
    // Checking is name available is horribly slow atm!
    bool AssetManager::nameAvailable(const std::string& name) const
    {
        for (const std::pair<UUID_t, Asset*> asset : _assets)
        {
            if (asset.second->getName() == name)
                return false;
        }
        return true;
    }

    bool AssetManager::validateAsset(
        const char* callLocation,
        UUID_t assetID,
        AssetType requiredType
    )
    {
        const std::string callLocationStr(callLocation);
        const std::string locationStr(PLATYPUS_CURRENT_FUNC_NAME);
        std::unordered_map<UUID_t, Asset*>::const_iterator it = _assets.find(assetID);
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

    void AssetManager::destroyDefaultAssets()
    {
        std::set<UUID_t> toErase;
        for (UUID_t id : _defaultAssets)
        {
            if (_assets[id] == _pErrorImage)
                _pErrorImage = nullptr;
            else if (_assets[id] == _pErrorTexture)
                _pErrorTexture = nullptr;
            else if (_assets[id] == _pErrorMaterial)
                _pErrorMaterial = nullptr;
            else if (_assets[id] == _pErrorModel)
                _pErrorModel = nullptr;
            else if (_assets[id] == _pErrorMesh)
                _pErrorMesh = nullptr;

            delete _assets[id];
            _assets[id] = nullptr;

            toErase.insert(id);
            _persistentAssets.erase(id);
        }

        for (UUID_t idToErase : toErase)
            _assets.erase(idToErase);

        _defaultAssets.clear();
    }
}
