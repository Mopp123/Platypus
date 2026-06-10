#include "Material.hpp"
#include "AssetManager.hpp"

#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"

#include "platypus/graphics/Device.hpp"
#include <vector>


namespace platypus
{
    // NOTE: All transparent materials use opaque pass's depth buffer as texture!
    Material::Material(
        size_t uuidPool,
        UUID_t blendmapTextureID,
        UUID_t* pDiffuseTextureIDs,
        UUID_t* pSpecularTextureIDs,
        UUID_t* pNormalTextureIDs,
        size_t diffuseTextureCount,
        size_t specularTextureCount,
        size_t normalTextureCount,
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
        bool persistent,
        const std::string& customVertexShaderFilename,
        const std::string& customFragmentShaderFilename
    ) :
        Asset(uuidPool, AssetType::ASSET_TYPE_MATERIAL, name, id, persistent),
        _blendmapTextureID(blendmapTextureID),
        _diffuseTextureCount(diffuseTextureCount),
        _specularTextureCount(specularTextureCount),
        _normalTextureCount(normalTextureCount),
        _castShadows(castShadows),
        _receiveShadows(receiveShadows),
        _transparent(transparent),
        _shadeless(shadeless),

        _customVertexShaderFilename(customVertexShaderFilename),
        _customFragmentShaderFilename(customFragmentShaderFilename)
    {
        validateTextureCounts();

        memset(_diffuseTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_specularTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_normalTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);

        memcpy(_diffuseTextureIDs, pDiffuseTextureIDs, sizeof(UUID_t) * _diffuseTextureCount);
        memcpy(_specularTextureIDs, pSpecularTextureIDs, sizeof(UUID_t) * _specularTextureCount);
        memcpy(_normalTextureIDs, pNormalTextureIDs, sizeof(UUID_t) * _normalTextureCount);

        _uniformBufferData.lightingProperties.x = specularStrength;
        _uniformBufferData.lightingProperties.y = shininess;
        _uniformBufferData.lightingProperties.z = _shadeless ? 1.0 : 0.0;
        _uniformBufferData.lightingProperties.w = 0.0f;
        _uniformBufferData.textureProperties.x = textureOffset.x;
        _uniformBufferData.textureProperties.y = textureOffset.y;
        _uniformBufferData.textureProperties.z = textureScale.x;
        _uniformBufferData.textureProperties.w = textureScale.y;
        // Make sure not to use SRGB textures for normal maps.
        // ...so u don't waste a fuckload of time figuring out why the normals are fucked again:D
        // NOTE: This probably should be checked in "no debug" too?
        #ifdef PLATYPUS_DEBUG
            AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
            for (size_t i = 0; i < _normalTextureCount; ++i)
            {
                Texture* pNormalTexture = (Texture*)pAssetManager->getAsset(_normalTextureIDs[i], AssetType::ASSET_TYPE_TEXTURE);
                if ((pNormalTexture->getImage()->getFormat() == ImageFormat::R8G8B8A8_SRGB))
                {
                    Debug::log(
                        "@Material::Material "
                        "Normal texture at index: " + std::to_string(i) + " "
                        "was sRGB! You probably don't want that if you're using this "
                        "for normal mapping!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
            }
        #endif

        createDescriptorSetLayout();

        // NOTE: Materials' pipelines gets created on demand via Batcher atm!

        createShaderResources();

        const size_t maxFramesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
            updateUniformBuffers(i);
    }

    Material::~Material()
    {
        destroyShaderResources();
        _descriptorSetLayout.destroy();

        std::unordered_map<uint32_t, MaterialPipelineData*>::iterator it;
        for (it = _pipelines.begin(); it !=_pipelines.end(); ++it)
        {
            // NOTE: Maybe should at least warn in this case?
            if (!it->second)
                continue;

            delete it->second;
        }
    }

    void Material::recreate(
        UUID_t blendmapTextureID,
        const std::vector<UUID_t>& diffuseTextureIDs,
        const std::vector<UUID_t>& specularTextureIDs,
        const std::vector<UUID_t>& normalTextureIDs,
        float specularStrength,
        float shininess,
        const Vector2f& textureOffset,
        const Vector2f& textureScale,
        bool castShadows,
        bool receiveShadows,
        bool transparent,
        bool shadeless
    )
    {
        _blendmapTextureID = blendmapTextureID;
        _diffuseTextureCount = diffuseTextureIDs.size();
        _specularTextureCount = specularTextureIDs.size();
        _normalTextureCount = normalTextureIDs.size();
        _castShadows = castShadows;
        _receiveShadows = receiveShadows;
        _transparent = transparent;
        _shadeless = shadeless;

        validateTextureCounts();

        memset(_diffuseTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_specularTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_normalTextureIDs, NULL_UUID, sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);

        memcpy(_diffuseTextureIDs, diffuseTextureIDs.data(), sizeof(UUID_t) * _diffuseTextureCount);
        memcpy(_specularTextureIDs, specularTextureIDs.data(), sizeof(UUID_t) * _specularTextureCount);
        memcpy(_normalTextureIDs, normalTextureIDs.data(), sizeof(UUID_t) * _normalTextureCount);

        _uniformBufferData.lightingProperties.x = specularStrength;
        _uniformBufferData.lightingProperties.y = shininess;
        _uniformBufferData.lightingProperties.z = _shadeless ? 1.0 : 0.0;
        _uniformBufferData.lightingProperties.w = 0.0f;
        _uniformBufferData.textureProperties.x = textureOffset.x;
        _uniformBufferData.textureProperties.y = textureOffset.y;
        _uniformBufferData.textureProperties.z = textureScale.x;
        _uniformBufferData.textureProperties.w = textureScale.y;

        // ISSUE!
        // *If recreating completely, need to solve descriptor set layouts for the pipelines of
        // each mesh type if exists!
        //  ->ATM SOLVED IN recreateExistingPipelines()
        //
        // *If used mesh didn't earlier have tangents and adding
        // a normal map, this gets fucked (same issues as with adding blendmap)
        //  ->figure out what to do!
        //
        // *If adding blendmap that didn't earlier exist
        //  ->this gets fucked, because recreateExistingPipelines(),
        //  doesn't solve new shaders that blendmapping requires!
        //      ->figure this out!
        //
        // *If using blendmap, shaders exists only for:
        //  -using diffuse+specular channels (bds)
        //  -using diffuse+specular+normal channels (bdsn)
        // +All of these require using ALL SLOTS for each used channel!
        //
        // TODO:
        // *Need some way to fail this gracefully or to query requirements in advance!
        // *Need to prevent adding Materials using normal maps to Renderable3D's using mesh
        // that doesn't have tangents?
        //
        // CONTINUE HERE!
        //
        // IDEAS:
        // *Most issues could be solved "quickly" by replacing MeshType with MeshPropertyFlagBits
        // and have pipelines for all unique sets of those flag combinations (having the key
        // of _pipelines be uint32_t of MeshPropertyFlagBits instead of MeshType)
        //  -> This still doesn't solve the issue that shaders don't exist for all of those
        //  combinations, but that can be addressed later...

        Device::wait_for_operations();
        destroyPipeline();
        _descriptorSetLayout.destroy();
        destroyShaderResources();

        createDescriptorSetLayout();

        recreateExistingPipelines();
        createShaderResources();

        const size_t maxFramesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
            updateUniformBuffers(i);

        // NOTE: IMPORTANT!
        // Need to recreate all batches using this material if its' descriptor sets changed!
        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        pMasterRenderer->getBatcher().freeBatches();
    }

    void Material::createPipeline(
        const RenderPass* pRenderPass,
        uint32_t meshPropertyFlags
    )
    {
        std::string vertexShaderFilename = _customVertexShaderFilename;
        if (vertexShaderFilename.empty())
        {
            vertexShaderFilename = getShaderFilename(
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                meshPropertyFlags
            );
        }
        std::string fragmentShaderFilename = _customFragmentShaderFilename;
        if (fragmentShaderFilename.empty())
        {
            fragmentShaderFilename = getShaderFilename(
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                meshPropertyFlags
            );
        }
        Debug::log(
            "@Material::createPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        std::unordered_map<uint32_t, MaterialPipelineData*>::const_iterator pipelineIt = _pipelines.find(meshPropertyFlags);
        if (pipelineIt != _pipelines.end())
        {
            if (pipelineIt->second != nullptr)
            {
                Debug::log(
                    "@Material::createPipeline "
                    "Pipeline data already exists for material with ID: " + std::to_string(getID()) + " "
                    "for mesh type: " + mesh_type_to_string(get_mesh_type(meshPropertyFlags)),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
        }

        // TODO: Unfuck below
        bool instanced = static_cast<bool>(meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::INSTANCED));
        bool skinned = static_cast<bool>(meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::TYPE_SKINNED));

        _pipelines[meshPropertyFlags] = new MaterialPipelineData;
        MaterialPipelineData* pMaterialPipelineData = _pipelines[meshPropertyFlags];

        Shader* pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        Shader* pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );
        pMaterialPipelineData->pVertexShader = pVertexShader;
        pMaterialPipelineData->pFragmentShader = pFragmentShader;

        MeshPropertyFlagBits meshType = get_mesh_type(meshPropertyFlags);
        bool meshHasTangents = meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::HAS_TANGENTS);
        VertexBufferLayout meshVertexBufferLayout;
        if (meshType == MeshPropertyFlagBits::TYPE_STATIC)
        {
            if (meshHasTangents)
                meshVertexBufferLayout = VertexBufferLayout::get_common_static_tangent_layout();
            else
                meshVertexBufferLayout = VertexBufferLayout::get_common_static_layout();
        }
        else if (meshType == MeshPropertyFlagBits::TYPE_SKINNED)
        {
            if (meshHasTangents)
                meshVertexBufferLayout = VertexBufferLayout::get_common_skinned_tangent_layout();
            else
                meshVertexBufferLayout = VertexBufferLayout::get_common_skinned_layout();
        }

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        // Figure out all used vertex buffer layouts in addition to the mesh layout
        std::vector<VertexBufferLayout> vertexBufferLayouts;
        pMasterRenderer->solveVertexBufferLayouts(
            meshVertexBufferLayout,
            instanced,
            skinned,
            false,
            vertexBufferLayouts
        );
        // Figure out all used descriptor set layouts in addition to the material's layout
        std::vector<DescriptorSetLayout> descriptorSetLayouts;
        pMasterRenderer->solveDescriptorSetLayouts(
            this,
            instanced,
            skinned,
            false,
            descriptorSetLayouts
        );

        size_t pushConstantsSize = 0;
        ShaderStageFlagBits pushConstantsStage = ShaderStageFlagBits::SHADER_STAGE_NONE;
        // If receiving shadows, need to provide shadow proj and view matrices as push constants!
        if (_receiveShadows)
        {
            pushConstantsSize = sizeof(Matrix4f) * 2;
            pushConstantsStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
        }

        pMaterialPipelineData->pPipeline = new Pipeline(
            pRenderPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
            pMaterialPipelineData->pVertexShader,
            pMaterialPipelineData->pFragmentShader,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            !_transparent, // Enable depth write
            _transparent ? DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL : DepthCompareOperation::COMPARE_OP_LESS,
            true, // Enable color blend
            pushConstantsSize, // Push constants size
            pushConstantsStage // Push constants' stage flags
        );
        pMaterialPipelineData->pPipeline->create();
    }

    void Material::recreateExistingPipelines()
    {
        bool created = false;

        std::unordered_map<uint32_t, MaterialPipelineData*>::iterator it;
        for (it = _pipelines.begin(); it !=_pipelines.end(); ++it)
        {
            // NOTE: Maybe should at least warn in this case?
            if (!it->second)
                continue;

            if (it->second->pPipeline)
            {
                MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
                std::vector<DescriptorSetLayout> descriptorSetLayouts;
                uint32_t meshPropertyFlags = it->first;
                bool skinned = meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::TYPE_SKINNED);
                bool instanced = meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::INSTANCED);
                pMasterRenderer->solveDescriptorSetLayouts(
                    this,
                    instanced, //instanced,
                    skinned, //skinned,
                    false,
                    descriptorSetLayouts
                );
                it->second->pPipeline->setDescriptorSetLayouts(descriptorSetLayouts);
                it->second->pPipeline->create();
                created = true;
            }
        }

        if (!created)
            warnUnassigned("@Material::recreateExistingPipeline");
    }

    void Material::destroyPipeline()
    {
        bool destroyed = false;

        std::unordered_map<uint32_t, MaterialPipelineData*>::iterator it;
        for (it = _pipelines.begin(); it !=_pipelines.end(); ++it)
        {
            // NOTE: Maybe should at least warn in this case?
            if (!it->second)
                continue;

            if (it->second->pPipeline)
            {
                it->second->pPipeline->destroy();
                destroyed = true;
            }
        }

        if (!destroyed)
            warnUnassigned("@Material::destroyPipeline");
    }

    void Material::createShaderResources()
    {
        if (!_uniformBuffers.empty())
        {
            Debug::log(
                "@Material::createShaderResources "
                "Uniform buffers already exists for Material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        if (!_descriptorSets.empty())
        {
            Debug::log(
                "@Material::createShaderResources "
                "Descriptor sets already exists for Material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // Omg this is soo fucking stupid DO SOMETHING ABOUT THIS!
        DescriptorPool* pDescriptorPool = Device::get_descriptor_pool();
        size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        _uniformBuffers.resize(framesInFlight);

        std::vector<Texture*> allTextures = getTextures();
        std::vector<DescriptorSetComponent> textureComponents(allTextures.size());
        for (size_t i = 0; i < allTextures.size(); ++i)
            textureComponents[i] = { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, allTextures[i] };

        // TODO: Do something about this convoluted mess
        if (_receiveShadows && _transparent)
        {
            Debug::log(
                "@Material::createShaderResources "
                "Material was receiving shadows and transparent which isn't currently allowed "
                "since shadowmap and scene's depth map is considered the last descriptor of material.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // TODO:
        // When findTextureDescriptorIndices() works, get rid of the below commented out stuff!
        findTextureDescriptorIndices();
        //if (_receiveShadows)
        //    _shadowmapDescriptorIndex = textureComponents.size() - 1;

        if (_transparent)
        {
            //_sceneDepthDescriptorIndex = textureComponents.size() - 1;
            // NOTE: ONLY TESTING ATM!
            textureComponents[_sceneDepthDescriptorIndex].depthImageTEST = true;
        }

        for (size_t i = 0; i < framesInFlight; ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                &_uniformBufferData,
                sizeof(MaterialUniformBufferData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _uniformBuffers[i] = pUniformBuffer;

            std::vector<DescriptorSetComponent> components = textureComponents;
            components.push_back({ DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pUniformBuffer });

            _descriptorSets.push_back(
                pDescriptorPool->createDescriptorSet(
                    _descriptorSetLayout,
                    components
                )
            );
        }
    }

    void Material::destroyShaderResources()
    {
        Device::wait_for_operations();

        for (Buffer* pBuffer : _uniformBuffers)
            delete pBuffer;

        _uniformBuffers.clear();

        DescriptorPool* pDescriptorPool = Device::get_descriptor_pool();
        pDescriptorPool->freeDescriptorSets(_descriptorSets);
        _descriptorSets.clear();
    }

    void Material::updateShadowmapDescriptorSet(Texture* pShadowmapTexture)
    {
        #ifdef PLATYPUS_DEBUG
            if (!_receiveShadows)
            {
                Debug::log(
                    "@Material::updateShadowmapDescriptorSet "
                    "Material not marked to receive shadows!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
        #endif
        updateDescriptorSetTexture(pShadowmapTexture, _shadowmapDescriptorIndex);
    }

    void Material::updateSceneDepthDescriptorSet(Texture* pSceneDepthTexture)
    {
        #ifdef PLATYPUS_DEBUG
            if (!_transparent)
            {
                Debug::log(
                    "@Material::updateSceneDepthDescriptorSet "
                    "Material not marked as transparent!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
        #endif
        updateDescriptorSetTexture(pSceneDepthTexture, _sceneDepthDescriptorIndex);
    }

    Texture* Material::getBlendmapTexture() const
    {
        if (_blendmapTextureID == NULL_UUID)
        {
            Debug::log(
                "@Material::getBlendmapTexture "
                "blendmap texture asset ID was NULL_ID.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _blendmapTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getDiffuseTexture(size_t slot) const
    {
        if (slot >= _diffuseTextureCount)
        {
            Debug::log(
                "@Material::getDiffuseTexture "
                "slot(" + std::to_string(slot) + ") out of bounds. "
                "Material has " + std::to_string(_diffuseTextureCount) + " diffuse textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _diffuseTextureIDs[slot],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getSpecularTexture(size_t slot) const
    {
        if (slot >= _specularTextureCount)
        {
            Debug::log(
                "@Material::getSpecularTexture "
                "slot(" + std::to_string(slot) + ") out of bounds. "
                "Material has " + std::to_string(_specularTextureCount) + " specular textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _specularTextureIDs[slot],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getNormalTexture(size_t slot) const
    {
        if (slot >= _normalTextureCount)
        {
            Debug::log(
                "@Material::getNormalTexture "
                "slot(" + std::to_string(slot) + ") out of bounds. "
                "Material has " + std::to_string(_normalTextureCount) + " normal textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _normalTextureIDs[slot],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    std::vector<Texture*> Material::getTextures() const
    {
        std::vector<Texture*> textures;
        if (_blendmapTextureID != NULL_UUID)
            textures.push_back(getBlendmapTexture());

        for (size_t i = 0; i < _diffuseTextureCount; ++i)
            textures.push_back(getDiffuseTexture(i));

        for (size_t i = 0; i < _specularTextureCount; ++i)
            textures.push_back(getSpecularTexture(i));

        for (size_t i = 0; i < _normalTextureCount; ++i)
            textures.push_back(getNormalTexture(i));

        if (_receiveShadows && _transparent)
        {
            Debug::log(
                "@Material::getTextures "
                "Material was receiving shadows and transparent. This currently isn't supported yet!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        if (_receiveShadows)
        {
            Texture* pShadowPassDepthAttachment = pMasterRenderer->getShadowPassInstance()->getFramebuffer(0)->getDepthAttachment();
            textures.push_back(pShadowPassDepthAttachment);
        }

        if (_transparent)
        {
            Texture* pDepthTexture = pMasterRenderer->getOpaqueFramebuffer()->getDepthAttachment();
            textures.push_back(pDepthTexture);
        }

        return textures;
    }

    UUID_t Material::getDiffuseTextureID(size_t slot) const
    {
        if (slot >= PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "Texture slot(" + std::to_string(slot) + ") out of bounds! "
                "Material can have up to " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + " textures per slot.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return _diffuseTextureIDs[slot];
    }

    UUID_t Material::getSpecularTextureID(size_t slot) const
    {
        if (slot >= PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "Texture slot(" + std::to_string(slot) + ") out of bounds! "
                "Material can have up to " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + " textures per slot.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return _specularTextureIDs[slot];
    }

    UUID_t Material::getNormalTextureID(size_t slot) const
    {
        if (slot >= PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "Texture slot(" + std::to_string(slot) + ") out of bounds! "
                "Material can have up to " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + " textures per slot.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return _normalTextureIDs[slot];
    }

    void Material::setLightingProperties(float specularStrength, float shininess, bool shadeless)
    {
        _uniformBufferData.lightingProperties.x = specularStrength;
        _uniformBufferData.lightingProperties.y = shininess;
        _uniformBufferData.lightingProperties.z = (float)shadeless;
        const size_t currentFrame = Application::get_instance()->getMasterRenderer()->getCurrentFrame();
        updateUniformBuffers(currentFrame);
    }

    void Material::setTextureProperties(const Vector2f& textureOffset, const Vector2f& textureScale)
    {
        _uniformBufferData.textureProperties.x = textureOffset.x;
        _uniformBufferData.textureProperties.y = textureOffset.y;
        _uniformBufferData.textureProperties.z = textureScale.x;
        _uniformBufferData.textureProperties.w = textureScale.y;
        //const size_t currentFrame = Application::get_instance()->getMasterRenderer()->getCurrentFrame();
        //updateUniformBuffers(currentFrame);
        const size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        Device::wait_for_operations();
        for (size_t i = 0; i < framesInFlight; ++i)
            updateUniformBuffers(i);
    }

    Pipeline* Material::getPipeline(uint32_t meshPropertyFlags)
    {
        std::unordered_map<uint32_t, MaterialPipelineData*>::iterator it = _pipelines.find(meshPropertyFlags);
        if (it != _pipelines.end())
            return it->second->pPipeline;

        return nullptr;
    }

    /*
        Serialized format (in order):
            ID_t assetID = NULL_ID;
            float specularStrength = 0.0f;
            float shininess = 0.0f;
            Vector2f textureOffset;
            Vector2f textureScale;
            uint8_t castShadows = 1;
            uint8_t receiveShadows = 1;
            uint8_t transparent = 0;
            uint8_t shadeless = 0;
            uint8_t persistent = 0;
            ID_t blendmapTextureID = NULL_ID;
            ID_t diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            ID_t specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            ID_t normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
            char name[metadata_name_size];
    */
    void Material::writeToMetadataBuffer(
        std::vector<char>& targetBuffer
    ) const
    {
        PLATYPUS_ASSERT(_name.size() <= asset_metadata_name_size);

        const size_t prevSize = targetBuffer.size();
        targetBuffer.resize(prevSize + get_serialized_metadata_size());
        char* pBuf = targetBuffer.data() + prevSize;

        memcpy(pBuf, &_id, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        const float specularStrength = getSpecularStrength();
        memcpy(pBuf + pos, &specularStrength, sizeof(float));
        pos += sizeof(float);

        const float specularShininess = getShininess();
        memcpy(pBuf + pos, &specularShininess, sizeof(float));
        pos += sizeof(float);

        const Vector2f textureOffset = getTextureOffset();
        memcpy(pBuf + pos, &textureOffset, sizeof(Vector2f));
        pos += sizeof(Vector2f);

        const Vector2f textureScale = getTextureScale();
        memcpy(pBuf + pos, &textureScale, sizeof(Vector2f));
        pos += sizeof(Vector2f);

        const uint8_t castShadows = static_cast<uint8_t>(_castShadows);
        memcpy(pBuf + pos, &castShadows, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t receiveShadows = static_cast<uint8_t>(_receiveShadows);
        memcpy(pBuf + pos, &receiveShadows, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t transparent = static_cast<uint8_t>(_transparent);
        memcpy(pBuf + pos, &transparent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t shadeless = static_cast<uint8_t>(_shadeless);
        memcpy(pBuf + pos, &shadeless, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        const uint8_t persistent = static_cast<const uint8_t>(_persistent);
        memcpy(pBuf + pos, &persistent, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(pBuf + pos, &_blendmapTextureID, sizeof(UUID_t));
        pos += sizeof(UUID_t);

        const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;
        const size_t textureChannelSize = sizeof(UUID_t) * texturesPerChannel;
        memcpy(pBuf + pos, &_diffuseTextureIDs, textureChannelSize);
        pos += textureChannelSize;
        memcpy(pBuf + pos, &_specularTextureIDs, textureChannelSize);
        pos += textureChannelSize;
        memcpy(pBuf + pos, &_normalTextureIDs, textureChannelSize);
        pos += textureChannelSize;

        memset(pBuf + pos, 0, asset_metadata_name_size);
        memcpy(pBuf + pos, _name.data(), _name.size());
        pos += asset_metadata_name_size;

        PLATYPUS_ASSERT(pos == get_serialized_metadata_size());
    }

    Material* Material::create_from_metadata_buffer(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    )
    {
        PLATYPUS_ASSERT((bufferPos  + get_serialized_metadata_size()) <= targetBuffer.size());

        UUID_t id;
        float specularStrength;
        float shininess;
        Vector2f textureOffset;
        Vector2f textureScale;
        uint8_t castShadows;
        uint8_t receiveShadows;
        uint8_t transparent;
        uint8_t shadeless;
        uint8_t persistent;
        UUID_t blendmapTextureID;
        UUID_t diffuseTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        UUID_t specularTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        UUID_t normalTextureIDs[PE_MAX_MATERIAL_TEX_CHANNELS];
        char name[asset_metadata_name_size];

        const char* pBuf = targetBuffer.data() + bufferPos;

        memcpy(&id, pBuf, sizeof(UUID_t));
        size_t pos = sizeof(UUID_t);

        memcpy(&specularStrength, pBuf + pos, sizeof(float));
        pos += sizeof(float);

        memcpy(&shininess, pBuf + pos, sizeof(float));
        pos += sizeof(float);

        memcpy(&textureOffset, pBuf + pos, sizeof(Vector2f));
        pos += sizeof(Vector2f);

        memcpy(&textureScale, pBuf + pos, sizeof(Vector2f));
        pos += sizeof(Vector2f);

        memcpy(&castShadows, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&receiveShadows, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&transparent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&shadeless, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&persistent, pBuf + pos, sizeof(uint8_t));
        pos += sizeof(uint8_t);

        memcpy(&blendmapTextureID, pBuf + pos, sizeof(UUID_t));
        pos += sizeof(UUID_t);

        const size_t texturesPerChannel = PE_MAX_MATERIAL_TEX_CHANNELS;
        const size_t textureChannelSize = sizeof(UUID_t) * texturesPerChannel;
        memcpy(&diffuseTextureIDs, pBuf + pos, textureChannelSize);
        pos += textureChannelSize;
        memcpy(&specularTextureIDs, pBuf + pos, textureChannelSize);
        pos += textureChannelSize;
        memcpy(&normalTextureIDs, pBuf + pos, textureChannelSize);
        pos += textureChannelSize;

        memcpy(&name, pBuf + pos, asset_metadata_name_size);
        pos += asset_metadata_name_size;

        PLATYPUS_ASSERT(pos == get_serialized_metadata_size());

        std::vector<UUID_t> diffuseTextures;
        std::vector<UUID_t> specularTextures;
        std::vector<UUID_t> normalTextures;
        for (size_t i = 0; i < texturesPerChannel; ++i)
        {
            if (diffuseTextureIDs[i] != NULL_UUID)
                diffuseTextures.push_back(diffuseTextureIDs[i]);
            if (specularTextureIDs[i] != NULL_UUID)
                specularTextures.push_back(specularTextureIDs[i]);
            if (normalTextureIDs[i] != NULL_UUID)
                normalTextures.push_back(normalTextureIDs[i]);
        }

        // TODO: Allow specifying custom shader files
        Material* pMaterial = pAssetManager->createMaterial(
            blendmapTextureID,
            diffuseTextures,
            specularTextures,
            normalTextures,
            specularStrength,
            shininess,
            textureOffset,
            textureScale,
            static_cast<bool>(castShadows),
            static_cast<bool>(receiveShadows),
            static_cast<bool>(transparent),
            static_cast<bool>(shadeless),
            std::string(name),
            id
        );

        if (persistent)
            pAssetManager->makePersistent(pMaterial);

        return pMaterial;
    }

    size_t Material::get_serialized_metadata_size()
    {
        return sizeof(UUID_t) +
            sizeof(float) * 2 +
            sizeof(Vector2f) * 2 +
            sizeof(uint8_t) * 5 +
            sizeof(UUID_t) +
            sizeof(UUID_t) * PE_MAX_MATERIAL_TEX_CHANNELS * 3 +
            asset_metadata_name_size;
    }

    void Material::warnUnassigned(const std::string& beginStr)
    {
        Debug::log(
            beginStr +
            " Pipeline data was nullptr. "
            "This Material may have been created but not used by any renderable component.",
            Debug::MessageType::PLATYPUS_WARNING
        );
    }

    // NOTE: Below setTexture funcs can atm ONLY be used if there was texture in specified slot earlier!
    // TODO: Allow adding new textures (requires some recreation system?)
    void Material::setBlendmapTexture(UUID_t textureID)
    {
        if (_blendmapTextureID == NULL_UUID)
        {
            Debug::log(
                "Currently this can be used only to update a texture to another "
                "(requires blendmap texture to already exist)",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (textureID == NULL_UUID)
        {
            Debug::log(
                "Given textureID was NULL_UUID",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        AssetManager* pAssetManger = Application::get_instance()->getAssetManager();
        Asset* pNewTextureAsset = pAssetManger->getAsset(textureID, AssetType::ASSET_TYPE_TEXTURE);
        if (!pNewTextureAsset)
        {
            Debug::log(
                "Failed to find textureID " + std::to_string(textureID),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        Texture* pNewTexture = reinterpret_cast<Texture*>(pNewTextureAsset);
        updateDescriptorSetTexture(pNewTexture, _blendmapTextureDescriptorIndex);
        _blendmapTextureID = textureID;
    }

    void Material::setDiffuseTexture(UUID_t textureID, size_t slot)
    {
        UUID_t* pTextures = _diffuseTextureIDs;
        UUID_t** ppTextures = &pTextures;
        setTexture(textureID, slot, _diffuseTextureDescriptorIndices[slot], ppTextures);
    }

    void Material::setSpecularTexture(UUID_t textureID, size_t slot)
    {
        UUID_t* pTextures = _specularTextureIDs;
        UUID_t** ppTextures = &pTextures;
        setTexture(textureID, slot, _specularTextureDescriptorIndices[slot], ppTextures);
    }

    void Material::setNormalTexture(UUID_t textureID, size_t slot)
    {
        UUID_t* pTextures = _normalTextureIDs;
        UUID_t** ppTextures = &pTextures;
        setTexture(textureID, slot, _normalTextureDescriptorIndices[slot], ppTextures);
    }

    void Material::setTexture(
        UUID_t textureID,
        size_t slot,
        uint32_t descriptorIndex,
        UUID_t** ppTextures
    )
    {
        if (slot > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "Texture slot " + std::to_string(slot) + " out of bounds! "
                "Max texture slots per channel is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if ((*ppTextures)[slot] == NULL_UUID)
        {
            Debug::log(
                "Currently this can be used only to update a texture to another "
                "(requires texture to already exist in slot " + std::to_string(slot) + ")",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (textureID == NULL_UUID)
        {
            Debug::log(
                "Given textureID was NULL_UUID",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        AssetManager* pAssetManger = Application::get_instance()->getAssetManager();
        Asset* pNewTextureAsset = pAssetManger->getAsset(textureID, AssetType::ASSET_TYPE_TEXTURE);
        if (!pNewTextureAsset)
        {
            Debug::log(
                "Failed to find textureID " + std::to_string(textureID),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        Texture* pNewTexture = reinterpret_cast<Texture*>(pNewTextureAsset);
        updateDescriptorSetTexture(pNewTexture, descriptorIndex);
        (*ppTextures)[slot]= textureID;
    }

    // NOTE: NOT TESTED, MIGHT BE FUCKED!!
    void Material::findTextureDescriptorIndices()
    {
        // NOTE: If blendmap is used its' descriptor index should always be 0!
        uint32_t baseIndex = _blendmapTextureID == NULL_UUID ? 0 : 1;
        for (size_t i = 0; i < PE_MAX_MATERIAL_TEX_CHANNELS; ++i)
        {
            _diffuseTextureDescriptorIndices[i] = baseIndex + i;
            _specularTextureDescriptorIndices[i] = baseIndex + _diffuseTextureCount + i;
            _normalTextureDescriptorIndices[i] = baseIndex + _diffuseTextureCount + _specularTextureCount + i;
        }
        size_t totalTextureCount = getTotalTextureCount();
        // NOTE: Material can't be transparent and receive shadows atm, so below is fine!
        if (_transparent)
            _sceneDepthDescriptorIndex = totalTextureCount - 1;
        if (_receiveShadows)
            _shadowmapDescriptorIndex = totalTextureCount - 1;
    }

    void Material::updateDescriptorSetTexture(Texture* pTexture, uint32_t descriptorIndex)
    {
        DescriptorPool* pDescriptorPool = Device::get_descriptor_pool();
        for (DescriptorSet& descriptorSet : _descriptorSets)
        {
            DescriptorSetComponent component{
                DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                pTexture
            };
            component.depthImageTEST = _transparent && descriptorIndex == _sceneDepthDescriptorIndex;
            descriptorSet.update(
                *pDescriptorPool,
                descriptorIndex,
                component
            );
        }
    }

    void Material::validateTextureCounts()
    {
        if (_diffuseTextureCount == 0)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "At least one diffuse texture required!"
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_diffuseTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many diffuse textures provided(" + std::to_string(_diffuseTextureCount) + ")! "
                "Max diffuse texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_specularTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many specular textures provided(" + std::to_string(_specularTextureCount) + ")! "
                "Max specular texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_normalTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many normal textures provided(" + std::to_string(_normalTextureCount) + ")! "
                "Max normal texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void Material::createDescriptorSetLayout()
    {
        std::vector<DescriptorSetLayoutBinding> layoutBindings;
        uint32_t textureBindingCount = getTotalTextureCount();
        for (uint32_t textureBinding = 0; textureBinding < textureBindingCount; ++textureBinding)
        {
            layoutBindings.push_back(
                {
                    textureBinding,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { } }
                }
            );
        }

        layoutBindings.push_back(
            {
                textureBindingCount,
                1,
                DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                {
                    { ShaderDataType::Float4 }, // x = specular strength, y = shininess, z = is shadeless, w = undefined for now
                    { ShaderDataType::Float4 } // x,y = texture offset, z,w = texture scale
                }
            }
        );
        _descriptorSetLayout = {
            layoutBindings
        };
    }

    // NOTE: This updates all uniform buffers for all possible frames in flight,
    // not sure should we be doing that here...
    void Material::updateUniformBuffers(size_t frame)
    {
        #ifdef PLATYPUS_DEBUG
        if (frame >= _uniformBuffers.size())
        {
            Debug::log(
                "@Material::updateUniformBuffers "
                "No uniform buffer exists for frame: " + std::to_string(frame) + " "
                "Uniform buffer count: " + std::to_string(_uniformBuffers.size()) + " "
                "Material ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        #endif
        _uniformBuffers[frame]->updateDeviceAndHost(
            &_uniformBufferData,
            sizeof(MaterialUniformBufferData),
            0
        );
    }

    // TODO: Make this convoluted mess cleaner!
    std::string Material::getShaderFilename(uint32_t shaderStage, uint32_t meshPropertyFlags)
    {
        // Vertex shader "name flags":
        //  t = use tangent input
        //  i = use instanced transforms input
        //
        // Fragment shader "name flags":
        //  b = use blendmap
        //  d = use diffuse map
        //  s = use specular map
        //  n = use normal map
        //  a = use depth map (a stands for "alpha", transparent pass)

        // Example shader names:
        // vertex shader: "StaticVertexShader", "StaticVertexShader_t", "StaticVertexShader_ti"
        // fragment shader: "StaticFragmentShader_d", "StaticFragmentShader_ds", "SkinnedFragmentShader_dsn"
        std::string shaderName = "";
        if (_receiveShadows)
        {
            shaderName += "receiveShadows/";
        }

        // NOTE: Non instanced static and skinned meshes could use the same fragment shaders?
        MeshPropertyFlagBits meshType = get_mesh_type(meshPropertyFlags);
        if (meshType == MeshPropertyFlagBits::TYPE_STATIC)
        {
            shaderName += "Static";
        }
        else if (meshType == MeshPropertyFlagBits::TYPE_SKINNED)
        {
            shaderName += "Skinned";
        }
        else
        {
            Debug::log(
                "No valid mesh type found from meshPropertyFlags",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // Using same vertex shader for diffuse and diffuse+specular
        // TODO: Make this less stupid
        bool meshHasTangents = meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::HAS_TANGENTS);
        bool instanced = meshPropertyFlags & static_cast<uint32_t>(MeshPropertyFlagBits::INSTANCED);
        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
        {
            shaderName += "VertexShader";
            if (meshHasTangents || instanced)
            {
                shaderName += "_";
                if (meshHasTangents)
                {
                    // t stands for tangent
                    shaderName += "t";
                }
                if (instanced)
                {
                    // i stands for instanced
                    shaderName += "i";
                }
            }
        }
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        {
            shaderName += "FragmentShader";
            // If using non instanced, we got one additional descriptor set containing
            // transformation matrix, etc. -> Need to use fragment shader that takes this into account!
            // ...FUCKING STUPID AND ANNOYING!
            if (instanced)
                shaderName += "_i_";
            else
                shaderName += "_";

            if (_blendmapTextureID != NULL_UUID)
                shaderName += "b";
            if (_diffuseTextureCount > 0)
                shaderName += "d";
            if (_specularTextureCount > 0)
                shaderName += "s";
            if (_normalTextureCount > 0)
                shaderName += "n";
            if (_transparent)
                shaderName += "a";
        }

        return shaderName;
    }
}
