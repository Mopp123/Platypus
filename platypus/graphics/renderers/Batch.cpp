#include "Batch.hpp"
#include "platypus/core/Application.h"
#include "platypus/assets/AssetManager.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    RenderPassType Batcher::s_availableRenderPasses[PLATYPUS_BATCHER_AVAILABLE_RENDER_PASSES] = {
        RenderPassType::SCENE_PASS,
        RenderPassType::SHADOW_PASS
    };

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
                        { ShaderDataType::Mat4 }
                    }
                }
            }
        );
    }

    Batcher::~Batcher()
    {
        destroyManagedPipelines();

        s_jointDescriptorSetLayout.destroy();
        s_terrainDescriptorSetLayout.destroy();
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

    BatchPipelineData* Batcher::createBatchPipelineData(
        const RenderPass* pRenderPass,
        const std::string& vertexShaderFilename,
        const std::string& fragmentShaderFilename,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
        size_t pushConstantsSize,
        ShaderStageFlagBits pushConstantsShaderStage
    )
    {
        BatchPipelineData* pPipelineData = new BatchPipelineData;
        pPipelineData = new BatchPipelineData;
        pPipelineData->pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        pPipelineData->pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );

        pPipelineData->pPipeline = new Pipeline(
            pRenderPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
            pPipelineData->pVertexShader,
            pPipelineData->pFragmentShader,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            true, // Enable color blend
            pushConstantsSize, // Push constants size
            pushConstantsShaderStage // Push constants' stage flags
        );
        pPipelineData->pPipeline->create();
        _managedPipelineData.push_back(pPipelineData);
        return pPipelineData;
    }

    void Batcher::addBatch(RenderPassType renderPassType, ID_t identifier, Batch* pBatch)
    {
        _identifierBatchMapping[renderPassType][identifier] = _batches[renderPassType].size();
        _batches[renderPassType].push_back(pBatch);
    }

    /*
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
    */

    void Batcher::updateDeviceSideBuffers(size_t currentFrame)
    {
        for (std::vector<BatchShaderResource>& batchResources : _allocatedShaderResources)
        {
            for (BatchShaderResource& resource : batchResources)
            {
                // NOTE: Do we need to really update the whole buffer if it's not used entirely?
                Buffer* pBuffer = resource.buffer[currentFrame];
                pBuffer->updateDevice(
                    pBuffer->accessData(),
                    pBuffer->getTotalSize(),
                    0
                );
            }
        }
    }

    void Batcher::resetForNextFrame()
    {
        std::unordered_map<RenderPassType, std::vector<Batch*>>::iterator batchIt;
        for (batchIt = _batches.begin(); batchIt != _batches.end(); ++batchIt)
        {
            for (Batch* pBatch : batchIt->second)
            {
                pBatch->instanceCount = 0;
                pBatch->repeatCount = 0;
            }
        }
    }

    void Batcher::freeBatches()
    {
        // Free batches themselves
        std::unordered_map<RenderPassType, std::vector<Batch*>>::iterator batchIt;
        for (batchIt = _batches.begin(); batchIt != _batches.end(); ++batchIt)
        {
            for (Batch* pBatch : batchIt->second)
                delete pBatch;
        }

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

    const DescriptorSetLayout& Batcher::get_joint_descriptor_set_layout()
    {
        return s_jointDescriptorSetLayout;
    }

    const DescriptorSetLayout& Batcher::get_terrain_descriptor_set_layout()
    {
        return s_terrainDescriptorSetLayout;
    }

    void Batcher::createSharedBatchInstancedBuffers(
        ID_t identifier,
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
        addToAllocatedShaderResources(identifier, outBuffers, {});
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

    Batch* Batcher::getBatch(RenderPassType renderPassType, ID_t identifier)
    {
        if (_batches.find(renderPassType) == _batches.end())
            return nullptr;

        std::unordered_map<RenderPassType, std::unordered_map<ID_t, size_t>>::iterator passBatchIndexIt = _identifierBatchMapping.find(renderPassType);
        if (passBatchIndexIt != _identifierBatchMapping.end())
        {
            std::unordered_map<ID_t, size_t>& passBatchIndices = passBatchIndexIt->second;
            std::unordered_map<ID_t, size_t>::iterator passBatchIt = passBatchIndices.find(identifier);
            if (passBatchIt != passBatchIndices.end())
            {
                size_t batchIndex = passBatchIt->second;
                return _batches[renderPassType][batchIndex];
            }
        }
        return nullptr;
    }

    // Returns all batches for a render pass
    const std::vector<Batch*>& Batcher::getBatches(RenderPassType renderPassType)
    {
        // NOTE: Not sure is this really okay??
        // ...Should be fine to just add the key here if it doesn't exist since the vector
        // remains empty if there was nothing?
        return _batches[renderPassType];
    }

    // TODO: Optimize!
    // Returns batches sharing the same ID for all render passes
    std::vector<Batch*> Batcher::getBatches(ID_t identifier)
    {
        std::vector<Batch*> batches;
        for (size_t i = 0; i < PLATYPUS_BATCHER_AVAILABLE_RENDER_PASSES; ++i)
        {
            Batch* pPassBatch = getBatch(s_availableRenderPasses[i], identifier);
            if (pPassBatch)
                batches.push_back(pPassBatch);
        }
        return batches;
    }

    BatchShaderResource* Batcher::getSharedBatchResource(ID_t batchID, size_t resourceIndex)
    {
        std::unordered_map<ID_t, size_t>::const_iterator it = _batchShaderResourceMapping.find(batchID);
        if (it != _batchShaderResourceMapping.end())
        {
            // TODO: Make safer!!!!
            std::vector<BatchShaderResource>& resources = _allocatedShaderResources[it->second];
            if (resourceIndex < resources.size())
                return &resources[resourceIndex];
        }
        return nullptr;
    }

    void Batcher::updateHostSideSharedResource(
        ID_t batchID,
        size_t resourceIndex,
        void* pData,
        size_t dataSize,
        size_t offset,
        size_t currentFrame
    )
    {
        std::unordered_map<ID_t, size_t>::iterator it = _batchShaderResourceMapping.find(batchID);
        if (it == _batchShaderResourceMapping.end())
        {
            Debug::log(
                "@Batcher::updateHostSideSharedResource "
                "No shared shader resource found for batchID: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        std::vector<BatchShaderResource>& resources = _allocatedShaderResources[it->second];
        if (resourceIndex >= resources.size())
        {
            Debug::log(
                "@Batcher::updateHostSideSharedResource "
                "resourceIndex(" + std::to_string(resourceIndex) + ") out of bounds! "
                "Batch(ID: " + std::to_string(batchID) + ") has " + std::to_string(resources.size()) + " resources.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        BatchShaderResource& resource = resources[resourceIndex];
        if (currentFrame >= resource.buffer.size())
        {
            Debug::log(
                "@Batcher::updateHostSideSharedResource "
                "No buffer exists for currentFrame(" + std::to_string(currentFrame) + ") "
                "Batch(ID: " + std::to_string(batchID) + ") has " + std::to_string(resource.buffer.size()) + " buffers "
                "for resource index " + std::to_string(resourceIndex),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        resource.buffer[currentFrame]->updateHost(
            pData,
            dataSize,
            offset
        );
        resource.requiresDeviceUpdate = true;
    }

    Pipeline* Batcher::getSuitableManagedPipeline(
        const std::string& vertexShaderFilename,
        const std::string& fragmentShaderFilename,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
        size_t pushConstantsSize,
        ShaderStageFlagBits pushConstantsShaderStage
    )
    {
        for (BatchPipelineData* pPipelineData : _managedPipelineData)
        {
            Pipeline* pPipeline = pPipelineData->pPipeline;
            const std::vector<VertexBufferLayout>& pipelineVertexBufferLayouts = pPipeline->getVertexBufferLayouts();
            const std::vector<DescriptorSetLayout>& pipelineDescriptorSetLayouts = pPipeline->getDescriptorSetLayouts();
            // NOTE: Not sure if this comparison works properly atm!
            if (pPipeline->getVertexShader()->getFilename() == vertexShaderFilename &&
                pPipeline->getFragmentShader()->getFilename() == fragmentShaderFilename &&
                pipelineVertexBufferLayouts == vertexBufferLayouts &&
                pipelineDescriptorSetLayouts == descriptorSetLayouts &&
                pPipeline->getPushConstantsSize() == pushConstantsSize &&
                pPipeline->getPushConstantsStageFlags() == pushConstantsShaderStage
            )
            {
                return pPipeline;
            }
        }

        return nullptr;
    }

    bool Batcher::validateBatchDoesntExist(
        const char* callLocation,
        RenderPassType renderPassType,
        ID_t batchID
    ) const
    {
        const std::string locationStr(callLocation);
        std::unordered_map<RenderPassType, std::unordered_map<ID_t, size_t>>::const_iterator passIt = _identifierBatchMapping.find(renderPassType);
        if (passIt != _identifierBatchMapping.end())
        {
            const std::unordered_map<ID_t, size_t>& passBatches = passIt->second;
            std::unordered_map<ID_t, size_t>::const_iterator passBatchIt = passBatches.find(batchID);
            if (passBatchIt != passBatches.end())
            {
                Debug::log(
                    "@Batcher::" + locationStr + " "
                    "Batch with identifier: " + std::to_string(batchID) + " already exists "
                    "for render pass: " + render_pass_type_to_string(renderPassType),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return false;
            }
        }
        return true;
    }

    void Batcher::destroyManagedPipelines()
    {
        for (BatchPipelineData* pPipelineData : _managedPipelineData)
            delete pPipelineData;

        _managedPipelineData.clear();
    }

    void Batcher::recreateManagedPipelines()
    {
        for (BatchPipelineData* pPipelineData : _managedPipelineData)
        {
            pPipelineData->pPipeline->destroy();
            pPipelineData->pPipeline->create();
        }
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
