#include "Batch.hpp"
#include "platypus/core/Application.h"
#include "platypus/assets/AssetManager.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    DescriptorSetLayout Batcher::s_jointDescriptorSetLayout;
    DescriptorSetLayout Batcher::s_terrainDescriptorSetLayout;

    Batcher::Batcher(
        MasterRenderer& masterRenderer,
        DescriptorPool& descriptorPool,
        size_t maxStaticBatchLength,
        size_t maxSkinnedBatchLength,
        size_t maxTerrainBatchLength,
        size_t maxSkinnedMeshJoints
    ) :
        _masterRendererRef(masterRenderer),
        _descriptorPoolRef(descriptorPool),
        _maxStaticBatchLength(maxStaticBatchLength),
        _maxSkinnedBatchLength(maxSkinnedBatchLength),
        _maxTerrainBatchLength(maxTerrainBatchLength),
        _maxSkinnedMeshJoints(maxSkinnedMeshJoints)
    {
        s_jointDescriptorSetLayout = DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { ShaderDataType::Mat4, (int)_maxSkinnedMeshJoints } }
                }
            }
        );

        s_terrainDescriptorSetLayout = DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { ShaderDataType::Mat4 },
                        { ShaderDataType::Float2 },
                    }
                }
            }
        );
    }

    Batcher::~Batcher()
    {
        std::unordered_map<ID_t, BatchShadowPassPipelineData*>::iterator shadowPipelineIt;
        for (shadowPipelineIt = _batchShadowmapPipelineData.begin(); shadowPipelineIt != _batchShadowmapPipelineData.end(); ++shadowPipelineIt)
            delete shadowPipelineIt->second;

        _batchShadowmapPipelineData.clear();

        s_jointDescriptorSetLayout.destroy();
        s_terrainDescriptorSetLayout.destroy();
    }

    ID_t Batcher::getBatchID(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);

        std::unordered_map<ID_t, size_t>::const_iterator indexIt = _identifierBatchMapping.find(identifier);
        if (indexIt != _identifierBatchMapping.end())
            return indexIt->first;

        return NULL_ID;
    }

    // JUST TESTING
    ID_t Batcher::createBatch(
        ID_t meshID,
        ID_t materialID,
        ComponentType renderableType,
        const RenderPass* pShadowPass,
        size_t shadowPushConstantsSize,
        void* pShadowPushConstants
    )
    {
        // TODO: Some better way to deal with these...
        const size_t requiredShadowPushConstantsSize = sizeof(Matrix4f) * 2;
        if (pShadowPushConstants != nullptr && shadowPushConstantsSize != requiredShadowPushConstantsSize)
        {
            Debug::log(
                "@Batcher::createBatch "
                "Invalid shadow push constants size: " + std::to_string(shadowPushConstantsSize) + " "
                "required size is " + std::to_string(requiredShadowPushConstantsSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        // *Assuming Material has ability to create its own shader resources by itself

        // 1.Make sure Mesh and Material assets exist?
        // 2.Create batch specific vertex buffer(s) if needed
        // 3.Figure out "unique batch shader resource layout" and create that
        // 4.Figure out how to combine the used descriptor sets
        // 5.Create the actual batch

        ID_t identifier = ID::hash(meshID, materialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier))
            return NULL_ID;

        const size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pMaterialPipeline = pMaterial->getPipeline(renderableType);
        BatchShadowPassPipelineData* pShadowPassPipelineData = nullptr;
        VertexBufferLayout meshVertexBufferLayout = pMesh->getVertexBufferLayout();
        Buffer* pMeshVertexBuffer = pMesh->getVertexBuffer();
        const Buffer* pMeshIndexBuffer = pMesh->getIndexBuffer();

        std::vector<std::vector<Buffer*>> dynamicVertexBuffers;

        size_t dynamicUniformBufferElementSize = 0;
        // For now all 3D rendering takes scene 3D descriptor sets
        std::vector<std::vector<DescriptorSet>> usedDescriptorSets = {
            _masterRendererRef.getScene3DDataDescriptorSets()
        };
        std::vector<std::vector<DescriptorSet>> usedShadowDescriptorSets;

        size_t maxBatchLength = 0;
        bool createShadowPassPipeline = false;
        // Create batch specific resources
        if (renderableType == ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE)
        {
            createShadowPassPipeline = true;
            dynamicVertexBuffers.resize(1);
            dynamicVertexBuffers[0].resize(framesInFlight);
            maxBatchLength = _maxStaticBatchLength;
            createBatchInstancedBuffers(
                identifier,
                sizeof(Matrix4f),
                maxBatchLength, // NOTE: This needs to change if instancing something else than static meshes!
                framesInFlight,
                dynamicVertexBuffers[0]
            );
        }
        else if (renderableType == ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE)
        {
            createShadowPassPipeline = true;
            maxBatchLength = _maxSkinnedBatchLength;
            dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
                sizeof(Matrix4f) * _maxSkinnedMeshJoints
            );
            // NOTE: Maybe should have separate funcs for creating resources
            // explicitly for different batches?
            std::vector<BatchShaderResource> shaderResources(1);
            createBatchShaderResources(
                framesInFlight,
                identifier,
                maxBatchLength,
                {
                    {
                        ShaderResourceType::ANY,
                        dynamicUniformBufferElementSize,
                        s_jointDescriptorSetLayout,
                        { }
                    }
                },
                shaderResources
            );
            for (const BatchShaderResource& createdResource : shaderResources)
            {
                usedDescriptorSets.push_back(createdResource.descriptorSet);
                usedShadowDescriptorSets.push_back(createdResource.descriptorSet);;
            }
        }
        else if (renderableType == ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE)
        {
            maxBatchLength = _maxTerrainBatchLength;
            dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
                sizeof(Matrix4f) + sizeof(Vector2f)
            );
            std::vector<BatchShaderResource> shaderResources(1);
            createBatchShaderResources(
                framesInFlight,
                identifier,
                maxBatchLength,
                {
                    {
                        ShaderResourceType::ANY,
                        dynamicUniformBufferElementSize,
                        s_terrainDescriptorSetLayout,
                        { }
                    }
                },
                shaderResources
            );
            for (const BatchShaderResource& createdResource : shaderResources)
                usedDescriptorSets.push_back(createdResource.descriptorSet);
        }

        // ONLY TESTING HERE ATM: Need to have different descriptor sets for shadowpass.. how?
        // -> Here excluding the Material descriptor sets...
        std::vector<std::vector<DescriptorSet>> combinedShadowPassDescriptorSets = combineUsedDescriptorSets(
            framesInFlight,
            usedShadowDescriptorSets
        );

        // For now all 3D batches have material
        usedDescriptorSets.push_back(pMaterial->getDescriptorSets());

        // TODO: Validate descriptor set counts against frames in flight!
        std::vector<std::vector<DescriptorSet>> combinedDescriptorSets = combineUsedDescriptorSets(
            framesInFlight,
            usedDescriptorSets
        );


        // TESTING CREATING OFFSCREEN PIPELINE FOR STATIC AND SKINNED BATCHES
        BatchPushConstantsData shadowPassPushConstantsData{
            0,
            ShaderStageFlagBits::SHADER_STAGE_NONE,
            { },
            nullptr
        };
        if (pShadowPushConstants)
        {
            shadowPassPushConstantsData = {
                shadowPushConstantsSize,
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                {
                    { ShaderDataType::Mat4 },
                    { ShaderDataType::Mat4 }
                },
                pShadowPushConstants
            };
        }
        if (createShadowPassPipeline)
        {
            pShadowPassPipelineData = createShadowPassPipelineData(
                identifier,
                pShadowPass,
                renderableType,
                meshVertexBufferLayout,
                shadowPassPushConstantsData,
                pMaterial
            );
        }

        Pipeline* pShadowmapPipeline = pShadowPassPipelineData ? pShadowPassPipelineData->pPipeline : nullptr;

        Batch* pBatch = new Batch{
            BatchType::TERRAIN,
            pMaterialPipeline,
            pShadowmapPipeline,//pMaterialShadowPipeline,
            combinedDescriptorSets,
            combinedShadowPassDescriptorSets,
            (uint32_t)dynamicUniformBufferElementSize, // dynamic uniform buffer element size
            { pMeshVertexBuffer },
            dynamicVertexBuffers, // dynamic/instanced vertex buffers
            pMeshIndexBuffer,
            { },
            shadowPassPushConstantsData,
            //ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, // push constants shader stage flags
            0, // repeat count
            0 // instance count,
        };
        /*
        Batch* pBatch = new Batch{
            BatchType::TERRAIN,
            pMaterialPipeline,
            pShadowmapPipeline,//pMaterialShadowPipeline,
            combinedDescriptorSets,
            combinedShadowPassDescriptorSets,
            (uint32_t)dynamicUniformBufferElementSize, // dynamic uniform buffer element size
            { pMeshVertexBuffer },
            dynamicVertexBuffers, // dynamic/instanced vertex buffers
            pMeshIndexBuffer,
            0, // push constants size
            { }, // push constant uniform infos
            nullptr, // push constants data
            ShaderStageFlagBits::SHADER_STAGE_NONE, // push constants shader stage flags
            0, // repeat count
            0 // instance count,
        };
        */

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);

        return identifier;
    }

    static std::string get_shadowpass_shader_name(
        ShaderStageFlagBits shaderStage,
        ComponentType renderableType
    )
    {
        std::string shaderName = "shadows/";
        switch (renderableType)
        {
            case ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE: shaderName += "Static"; break;
            case ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE: shaderName += "Skinned"; break;
            case ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE: shaderName += "Terrain"; break;
            default:
            {
                Debug::log(
                    "@(Batcher)get_shadowpass_shader_name "
                    "Invalid renderable component type: " + component_type_to_string(renderableType),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
        }
        switch (shaderStage)
        {
            case ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT: shaderName += "Vertex"; break;
            case ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT: shaderName += "Fragment"; break;
            default:
            {
                Debug::log(
                    "@(Batcher)get_shadowpass_shader_name "
                    "Invalid shader stage: " + shader_stage_to_string(shaderStage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
        }
        return shaderName + "Shader";
    }

    BatchShadowPassPipelineData* Batcher::createShadowPassPipelineData(
        ID_t identifier,
        const RenderPass* pShadowPass,
        ComponentType renderableType,
        const VertexBufferLayout& meshVertexBufferLayout,
        const BatchPushConstantsData& pushConstantsData,
        const Material* pMaterial // JUST TESTING HERE: TODO: Remove
    )
    {
        BatchShadowPassPipelineData* pShadowPassPipelineData = new BatchShadowPassPipelineData;
        pShadowPassPipelineData = new BatchShadowPassPipelineData;
        pShadowPassPipelineData->pVertexShader = new Shader(
            get_shadowpass_shader_name(ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, renderableType),
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        pShadowPassPipelineData->pFragmentShader = new Shader(
            get_shadowpass_shader_name(ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, renderableType),
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );

        bool instanced = renderableType == ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE;
        bool skinned = renderableType == ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE;

        std::vector<VertexBufferLayout> vertexBufferLayouts;
        _masterRendererRef.solveVertexBufferLayouts(
            meshVertexBufferLayout,
            instanced,
            skinned,
            true,
            vertexBufferLayouts
        );
        std::vector<DescriptorSetLayout> descriptorSetLayouts;
        _masterRendererRef.solveDescriptorSetLayouts(
            pMaterial,
            skinned,
            true,
            descriptorSetLayouts
        );
        pShadowPassPipelineData->pPipeline = new Pipeline(
            pShadowPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
            pShadowPassPipelineData->pVertexShader,
            pShadowPassPipelineData->pFragmentShader,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            true, // Enable color blend
            pushConstantsData.size, // Push constants size
            pushConstantsData.shaderStage // Push constants' stage flags
        );
        pShadowPassPipelineData->pPipeline->create();
        _batchShadowmapPipelineData[identifier] = pShadowPassPipelineData;
        return pShadowPassPipelineData;
    }

    void Batcher::addToStaticBatch(
        ID_t identifier,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);
        if (!pBatch)
            return;

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of thing where multiple batches can exist for the same identifier
        if (pBatch->instanceCount >= _maxStaticBatchLength)
        {
            Debug::log(
                "@Batcher::addToStaticBatch "
                "Batch is full. Maximum static batch length is " + std::to_string(_maxStaticBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Buffer* pInstancedBuffer = getBatchBuffer(identifier, 0, currentFrame);
        pInstancedBuffer->updateHost(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            sizeof(Matrix4f) * pBatch->instanceCount
        );
        ++pBatch->instanceCount;
        pBatch->repeatCount = 1;
    }

    void Batcher::addToSkinnedBatch(
        ID_t identifier,
        void* pJointData,
        size_t jointDataSize,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->repeatCount >= _maxSkinnedBatchLength)
        {
            Debug::log(
                "@Batcher::addToSkinnedBatch "
                "Batch is full. Maximum skinned batch length is " + std::to_string(_maxSkinnedBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Buffer* pJointBuffer = getBatchBuffer(identifier, 0, currentFrame);
        uint32_t jointBufferOffset = pBatch->repeatCount * pJointBuffer->getDataElemSize();
        pJointBuffer->updateHost(
            pJointData,
            jointDataSize,
            jointBufferOffset
        );
        pBatch->instanceCount = 1;
        ++pBatch->repeatCount;
    }

    void Batcher::addToTerrainBatch(
        ID_t identifier,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->repeatCount >= _maxTerrainBatchLength)
        {
            Debug::log(
                "@Batcher::addToTerrainBatch "
                "Batch is full. Maximum skinned batch length is " + std::to_string(_maxSkinnedBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        const size_t terrainDataSize = sizeof(Matrix4f) + sizeof(Vector2f);
        std::vector<PE_byte> terrainData(terrainDataSize);
        memcpy(terrainData.data(), &transformationMatrix, sizeof(Matrix4f));
        // TODO: Some better way to deal with the tex coord multiplying
        const float tileSize = 1.0f;
        memcpy(terrainData.data() + sizeof(Matrix4f), &tileSize, sizeof(float));
        const float fVerticesPerRow = 2.0f;
        memcpy(terrainData.data() + sizeof(Matrix4f) + sizeof(float), &fVerticesPerRow, sizeof(float));

        Buffer* pTerrainBuffer = getBatchBuffer(identifier, 0, currentFrame);
        uint32_t terrainBufferOffset = pBatch->repeatCount * pTerrainBuffer->getDataElemSize();
        pTerrainBuffer->updateHost(
            terrainData.data(),
            terrainDataSize,
            terrainBufferOffset
        );

        pBatch->instanceCount = 1;
        ++pBatch->repeatCount;
    }

    void Batcher::updateDeviceSideBuffers(size_t currentFrame)
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        std::unordered_map<ID_t, size_t>::const_iterator it;
        for (it = _batchShaderResourceMapping.begin(); it != _batchShaderResourceMapping.end(); ++it)
        {
            // TODO: Make safer!
            const ID_t batchID = it->first;
            const Batch* pBatch = _batches[_identifierBatchMapping[batchID]];
            if (pBatch->instanceCount > 0 || pBatch->repeatCount > 0)
            {
                // TODO: Make safer!
                const size_t resourceIndex = _batchShaderResourceMapping[batchID];
                for (BatchShaderResource& shaderResource : _allocatedShaderResources[resourceIndex])
                {
                    Buffer* pBuffer = shaderResource.buffer[currentFrame];
                    pBuffer->updateDevice(
                        pBuffer->accessData(),
                        pBuffer->getTotalSize(),
                        0
                    );
                }
            }
        }
    }

    void Batcher::resetForNextFrame()
    {
        for (Batch* pBatch : _batches)
        {
            pBatch->repeatCount = 0;
            pBatch->instanceCount = 0;
        }
    }

    void Batcher::freeBatches()
    {
        // Free batches themselves
        for (Batch* pBatch : _batches)
            delete pBatch;

        _batches.clear();
        _identifierBatchMapping.clear();

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();
        for (std::vector<BatchShaderResource>& batchResources : _allocatedShaderResources)
        {
            for (BatchShaderResource& resource : batchResources)
            {
                for (Buffer* pBuffer : resource.buffer)
                    delete pBuffer;

                descriptorPool.freeDescriptorSets(resource.descriptorSet);
            }
        }
        _allocatedShaderResources.clear();
        _batchShaderResourceMapping.clear();
    }

    const std::vector<Batch*>& Batcher::getBatches() const
    {
        return _batches;
    }

    const DescriptorSetLayout& Batcher::get_joint_descriptor_set_layout()
    {
        return s_jointDescriptorSetLayout;
    }

    const DescriptorSetLayout& Batcher::get_terrain_descriptor_set_layout()
    {
        return s_terrainDescriptorSetLayout;
    }

    void Batcher::createBatchInstancedBuffers(
        ID_t batchID,
        size_t bufferElementSize,
        size_t maxBatchLength,
        size_t framesInFlight,
        std::vector<Buffer*>& outBuffers
    )
    {
        std::vector<char> bufferData(bufferElementSize * maxBatchLength);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            outBuffers[i] = new Buffer(
                bufferData.data(),
                bufferElementSize,
                maxBatchLength,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM,
                true
            );
        }
        addToAllocatedShaderResources(batchID, outBuffers, {});
    }

    void Batcher::createBatchShaderResources(
        size_t framesInFlight,
        ID_t batchID,
        size_t maxBatchLength,
        const std::vector<ShaderResourceLayout>& resourceLayouts,
        std::vector<BatchShaderResource>& outResources
    )
    {
        for (size_t resourceIndex = 0; resourceIndex < outResources.size(); ++resourceIndex)
        {
            const ShaderResourceLayout& resourceLayout = resourceLayouts[resourceIndex];
            BatchShaderResource& outResource = outResources[resourceIndex];
            outResource.type = resourceLayout.type;
            outResource.buffer.resize(framesInFlight);
            outResource.descriptorSet.resize(framesInFlight);
            for (size_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
            {
                const DescriptorSetLayout& descriptorSetLayout = resourceLayout.descriptorSetLayout;
                const std::vector<DescriptorSetLayoutBinding>& descriptorSetLayoutBindings = descriptorSetLayout.getBindings();
                std::vector<DescriptorSetComponent> descriptorSetComponents(descriptorSetLayoutBindings.size());
                size_t useTextureIndex = 0;
                for (size_t i = 0; i < descriptorSetComponents.size(); ++i)
                {
                    const DescriptorSetLayoutBinding& binding = descriptorSetLayoutBindings[i];
                    const DescriptorType& descriptorType = binding.getType();
                    const std::vector<Texture*>& textures = resourceLayout.textures;
                    if (descriptorType == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        if (useTextureIndex >= textures.size())
                        {
                            Debug::log(
                                "@Batcher::createBatchShaderResources "
                                "Descriptor set layout binding was: DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER "
                                "at index: " + std::to_string(i) + " "
                                "but no texture provided(useTextureIndex = " + std::to_string(useTextureIndex) + ")",
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            PLATYPUS_ASSERT(false);
                            return;
                        }

                        descriptorSetComponents[i] = { descriptorType, textures[useTextureIndex] };
                        ++useTextureIndex;
                    }
                    else if (descriptorType == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                            descriptorType == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER
                            )
                    {
                        const size_t bufferElementSize = resourceLayout.uniformBufferElementSize;
                        std::vector<char> bufferData(bufferElementSize * maxBatchLength);
                        memset(bufferData.data(), 0, bufferData.size());
                        Buffer* pUniformBuffer = new Buffer(
                            bufferData.data(),
                            bufferElementSize,
                            maxBatchLength,
                            BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                            true
                        );
                        outResource.buffer[frameIndex] = pUniformBuffer;

                        descriptorSetComponents[i] = { descriptorType, pUniformBuffer };
                    }
                }

                outResource.descriptorSet[frameIndex] = _descriptorPoolRef.createDescriptorSet(
                    descriptorSetLayout,
                    descriptorSetComponents
                );
            }
        }
        addToAllocatedShaderResources(batchID, outResources);
    }

    std::vector<std::vector<DescriptorSet>> Batcher::combineUsedDescriptorSets(
        size_t framesInFlight,
        const std::vector<std::vector<DescriptorSet>>& descriptorSetsToUse
    )
    {
        if (descriptorSetsToUse.empty())
            return { };

        std::vector<std::vector<DescriptorSet>> combinedDescriptorSets(framesInFlight);
        for (size_t frame = 0; frame < framesInFlight; ++frame)
        {
            for (size_t descriptorSetIndex = 0; descriptorSetIndex < descriptorSetsToUse.size(); ++descriptorSetIndex)
            {
                combinedDescriptorSets[frame].push_back(descriptorSetsToUse[descriptorSetIndex][frame]);
            }
        }
        return combinedDescriptorSets;
    }

    Batch* Batcher::getBatch(ID_t batchID)
    {
        std::unordered_map<ID_t, size_t>::iterator it = _identifierBatchMapping.find(batchID);
        if (it == _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::getBatch "
                "Failed to find batch using identifier: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _batches[it->second];
    }

    Buffer* Batcher::getBatchBuffer(ID_t batchID, size_t resourceIndex, size_t frame)
    {
        std::unordered_map<ID_t, size_t>::const_iterator it = _batchShaderResourceMapping.find(batchID);
        if (it == _batchShaderResourceMapping.end())
        {
            Debug::log(
                "@Batcher::getBatchBuffer "
                "Failed to find batch shader resource using identifier: " + std::to_string(batchID) + " "
                "resourceIndex = " + std::to_string(resourceIndex),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        // TODO: Make safer?
        return _allocatedShaderResources[it->second][resourceIndex].buffer[frame];
    }

    bool Batcher::validateBatchDoesntExist(const char* callLocation, ID_t batchID) const
    {
        const std::string locationStr(callLocation);
        if (_identifierBatchMapping.find(batchID) != _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::" + locationStr +" "
                "Batch already exists for identifier: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    bool Batcher::validateDescriptorSetCounts(
        const char* callLocation,
        size_t framesInFlight,
        size_t commonDescriptorSetCount,
        size_t materialDescriptorSetCount
    ) const
    {
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != materialDescriptorSetCount))
        {
            std::string locationStr(callLocation);
            Debug::log(
                "@Batcher::" + locationStr + "(validateDescriptorSetCounts) "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    bool Batcher::validateDescriptorSetCounts(
        const char* callLocation,
        size_t framesInFlight,
        size_t commonDescriptorSetCount,
        size_t batchDescriptorSetCount,
        size_t materialDescriptorSetCount
    ) const
    {
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != batchDescriptorSetCount) ||
             (commonDescriptorSetCount != materialDescriptorSetCount) ||
             (materialDescriptorSetCount != batchDescriptorSetCount))
        {
            std::string locationStr(callLocation);
            Debug::log(
                "@Batcher::" + locationStr + "(validateDescriptorSetCounts) "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Batch descriptor set count: " + std::to_string(batchDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    void Batcher::addToAllocatedShaderResources(
        ID_t batchID,
        std::vector<BatchShaderResource>& shaderResources
    )
    {
        if (_batchShaderResourceMapping.find(batchID) == _batchShaderResourceMapping.end())
        {
            _batchShaderResourceMapping[batchID] = _allocatedShaderResources.size();
            _allocatedShaderResources.push_back(shaderResources);
        }
        else
        {
            std::vector<BatchShaderResource>& existingResources = _allocatedShaderResources[_batchShaderResourceMapping[batchID]];
            existingResources.insert(existingResources.end(), shaderResources.begin(), shaderResources.end());
        }
    }

    // TODO: delete below after fixing static and skinned batches
    void Batcher::addToAllocatedShaderResources(
        ID_t batchID,
        const std::vector<Buffer*>& buffers,
        const std::vector<DescriptorSet> descriptorSets
    )
    {
        if (_batchShaderResourceMapping.find(batchID) == _batchShaderResourceMapping.end())
        {
            _batchShaderResourceMapping[batchID] = _allocatedShaderResources.size();
            _allocatedShaderResources.push_back({ { ShaderResourceType::ANY, buffers, descriptorSets } });
        }
        else
        {
            _allocatedShaderResources[_batchShaderResourceMapping[batchID]].push_back({ ShaderResourceType::ANY, buffers, descriptorSets });
        }
    }
}
