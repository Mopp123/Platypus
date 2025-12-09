#include "Batch.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    RenderPassType Batcher::s_availableRenderPasses[PLATYPUS_BATCHER_AVAILABLE_RENDER_PASSES] = {
        RenderPassType::SCREEN_PASS,
        RenderPassType::SHADOW_PASS
    };

    DescriptorSetLayout Batcher::s_staticDescriptorSetLayout;
    DescriptorSetLayout Batcher::s_jointDescriptorSetLayout;

    Batcher::Batcher(
        MasterRenderer& masterRenderer,
        DescriptorPool& descriptorPool,
        size_t maxStaticBatchLength,
        size_t maxStaticInstancedBatchLength,
        size_t maxSkinnedBatchLength,
        size_t maxSkinnedMeshJoints
    ) :
        _masterRendererRef(masterRenderer),
        _descriptorPoolRef(descriptorPool),
        _maxStaticBatchLength(maxStaticBatchLength),
        _maxStaticInstancedBatchLength(maxStaticInstancedBatchLength),
        _maxSkinnedBatchLength(maxSkinnedBatchLength),
        _maxSkinnedMeshJoints(maxSkinnedMeshJoints)
    {
        s_staticDescriptorSetLayout = DescriptorSetLayout(
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

        // Create batch templates
        const size_t staticBatchDynamicUBOElemSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f)
        );
        _batchTemplates[MeshType::MESH_TYPE_STATIC] = {
            _maxStaticBatchLength, // maxBatchLength
            (uint32_t)_maxStaticBatchLength, // maxRepeatCount
            1, // repeatAdvance,
            1, // maxInstanceCount,
            0, // instanceAdvance,
            0, // instance buffer elem size
            {
                {
                    ShaderResourceType::ANY,
                    staticBatchDynamicUBOElemSize,
                    s_staticDescriptorSetLayout,
                    { }
                }
            } // uniform resource layouts
        };

        _batchTemplates[MeshType::MESH_TYPE_STATIC_INSTANCED] = {
            _maxStaticInstancedBatchLength, // maxBatchLength
            1, // maxRepeatCount,
            0, // repeatAdvance,
            (uint32_t)_maxStaticInstancedBatchLength, // maxInstanceCount,
            1, // instanceAdvance,
            sizeof(Matrix4f), // instance buffer elem size
            { } // uniform resource layouts
        };

        const size_t dynamicJointBufferElemSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * _maxSkinnedMeshJoints
        );
        _batchTemplates[MeshType::MESH_TYPE_SKINNED] = {
            _maxSkinnedBatchLength,
            (uint32_t)_maxSkinnedBatchLength, // maxRepeatCount,
            1, // repeatAdvance,
            1, // maxInstanceCount,
            0, // instanceAdvance,
            0, // instance buffer elem size
            {
                {
                    ShaderResourceType::ANY,
                    dynamicJointBufferElemSize,
                    s_jointDescriptorSetLayout,
                    { }
                }
            }, // uniform resource layouts
        };
    }

    Batcher::~Batcher()
    {
        destroyManagedPipelines();

        s_staticDescriptorSetLayout.destroy();
        s_jointDescriptorSetLayout.destroy();
    }

    static std::string get_shadowpass_shader_name(
        ShaderStageFlagBits shaderStage,
        MeshType meshType
    )
    {
        std::string shaderName = "shadows/";
        std::string extension;

        switch (meshType)
        {
            case MeshType::MESH_TYPE_STATIC:
                shaderName += "Static";
                break;
            case MeshType::MESH_TYPE_STATIC_INSTANCED:
                shaderName += "Static";
                extension = shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT ? "_i" : "";
            break;
            case MeshType::MESH_TYPE_SKINNED:
                shaderName += "Skinned";
                break;
            default:
            {
                Debug::log(
                    "@(Batcher)get_shadowpass_shader_name "
                    "Invalid mesh type: " + mesh_type_to_string(meshType),
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
        return shaderName + "Shader" + extension;
    }

    BatchPipelineData* Batcher::createBatchPipelineData(
        const RenderPass* pRenderPass,
        const std::string& vertexShaderFilename,
        const std::string& fragmentShaderFilename,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
        CullMode cullMode,
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
            cullMode,
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
        destroyManagedPipelines();

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

    const DescriptorSetLayout& Batcher::get_static_descriptor_set_layout()
    {
        return s_staticDescriptorSetLayout;
    }

    const DescriptorSetLayout& Batcher::get_joint_descriptor_set_layout()
    {
        return s_jointDescriptorSetLayout;
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

    bool Batcher::batchResourcesExist(ID_t batchID) const
    {
        std::unordered_map<ID_t, size_t>::const_iterator indexIt = _batchShaderResourceMapping.find(batchID);
        if (indexIt == _batchShaderResourceMapping.end())
        {
            Debug::log(
                "@Batcher::batchResourcesExist "
                "No resource indices found for batchID: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_WARNING
            );
            return false;
        }
        if (indexIt->second >= _allocatedShaderResources.size())
        {
            Debug::log(
                "@Batcher::batchResourcesExist "
                "Found resource index (" + std::to_string(indexIt->second) + ") out of bounds "
                "using batchID: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_WARNING
            );
            return false;
        }
        return true;
    }

    // NOTE: Resources needs to exist if calling this! (check that with batchResourcesExist)
    std::vector<BatchShaderResource>& Batcher::accessSharedBatchResources(ID_t batchID)
    {
        return _allocatedShaderResources[_batchShaderResourceMapping[batchID]];
    }

    // NOTE: Not used anymore? TODO: Delete?
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

    void Batcher::createBatch(
        ID_t meshID,
        ID_t materialID,
        size_t maxBatchLength,
        uint32_t maxRepeatCount,
        uint32_t repeatAdvance,
        uint32_t maxInstanceCount,
        uint32_t instanceAdvance,
        size_t instanceBufferElementSize,
        const std::vector<ShaderResourceLayout>& uniformResourceLayouts,
        const Light * const pDirectionalLight,
        const RenderPass* pRenderPass
    )
    {
        ID_t batchID = ID::hash(meshID, materialID);
        RenderPassType renderPassType = pRenderPass->getType();
        if (!validateBatchDoesntExist("Batcher::createBatch", renderPassType, batchID))
            return;

        // TODO: Allow using both, repeat and instance advances.
        // This requires marking individual shared shader resources to be using one or the other
        // to be able to advance the buffer offset correctly when adding to this batch!
        if (instanceAdvance > 0 && repeatAdvance > 0)
        {
            Debug::log(
                "@Batcher::createBatch "
                "Batch was attempting to use both, repeat and instance advance. "
                "Currently batches can use only one or the other!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        MeshType meshType = pMesh->getType();

        // Currently batching requires valid material in order to figure out some unique batch ID.
        // TODO: Allow creating batches without materials?
        if (materialID == NULL_ID)
        {
            Debug::log(
                "@Batcher::createBatch "
                "materialID was NULL_ID! "
                "Currently batching requires a material.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        if (!pMaterial)
        {
            Debug::log(
                "@Batcher::createBatch "
                "Material with ID: " + std::to_string(materialID) + " was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Pipeline* pPipeline = nullptr;
        bool receivesShadows = false;
        const bool shadowPass = renderPassType == RenderPassType::SHADOW_PASS;

        size_t pushConstantsSize = 0;
        std::vector<UniformInfo> pushConstantsUniformInfos;
        ShaderStageFlagBits pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_NONE;
        void* pPushConstantsData = nullptr;

        // For now all 3D rendering takes scene3DDescriptorSets IF NOT SHADOWPASS
        std::vector<std::vector<DescriptorSet>> usedDescriptorSets;
        if (!shadowPass)
            usedDescriptorSets.push_back(_masterRendererRef.getScene3DDataDescriptorSets());

        // Use the material's pipeline and descriptor sets if not shadowpass
        if (pMaterial && !shadowPass)
        {
            // Create material pipeline if doesn't exist
            if (!pMaterial->getPipeline(meshType))
                pMaterial->createPipeline(pRenderPass, meshType);

            pPipeline = pMaterial->getPipeline(meshType);
            receivesShadows = pMaterial->receivesShadows();
        }

        const size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();

        // Dynamic vertex buffers
        //
        // NOTE: Atm allowing just a single dynamic vertex buffer for each frame in flight
        // per batch!
        // TODO: Allow more?
        std::vector<std::vector<Buffer*>> dynamicVertexBuffers;
        if (instanceBufferElementSize > 0)
        {
            dynamicVertexBuffers.push_back(
                getOrCreateSharedInstancedBuffer(
                    batchID,
                    instanceBufferElementSize,
                    maxBatchLength,
                    framesInFlight
                )
            );
        }

        // Provide shadow proj and view matrices as push constants if needed
        if (receivesShadows || shadowPass)
        {
            if (!pDirectionalLight)
            {
                Debug::log(
                    "@Batcher::createBatch "
                    "Batch requires directional light but provided directional light was nullptr!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            // TODO: Better way of handling this
            pushConstantsSize = sizeof(Matrix4f) * 2;
            pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
            pushConstantsUniformInfos = {
                { ShaderDataType::Mat4 },
                { ShaderDataType::Mat4 }
            };
            pPushConstantsData = (void*)pDirectionalLight;
        }

        // Shared shader resources (uniform buffers + descriptor sets)
        uint32_t dynamicUniformBufferElementSize = 0;
        if (!uniformResourceLayouts.empty())
        {
            // TODO: Allow multiple shared shader resources for batch
            if (uniformResourceLayouts.size() != 1)
            {
                Debug::log(
                    "@Batcher::createBatch "
                    "Currently supporting only a single dynamic shared resource per batch! "
                    "Provided resource layouts: " + std::to_string(uniformResourceLayouts.size()),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            std::vector<BatchShaderResource>& createdShaderResources = getOrCreateSharedShaderResources(
                batchID,
                maxBatchLength,
                uniformResourceLayouts,
                framesInFlight
            );
            dynamicUniformBufferElementSize = uniformResourceLayouts[0].uniformBufferElementSize;
            for (BatchShaderResource& resource : createdShaderResources)
                usedDescriptorSets.push_back(resource.descriptorSet);
        }

        // Need to add the Material descriptor sets last if using those..
        if (materialID != NULL_ID && !shadowPass)
            usedDescriptorSets.push_back(pMaterial->getDescriptorSets());

        std::vector<std::vector<DescriptorSet>> combinedDescriptorSets = combineUsedDescriptorSets(
            framesInFlight,
            usedDescriptorSets
        );

        // Create pipeline if not using Material pipeline.
        // NOTE: Currently this is used ONLY for SHADOWPASS shaders.
        // Allow some more flexible way of creating pipelines without Materials?
        if (!pPipeline)
        {
            std::vector<VertexBufferLayout> usedVertexBufferLayouts;
            _masterRendererRef.solveVertexBufferLayouts(
                pMesh->getVertexBufferLayout(),
                meshType == MeshType::MESH_TYPE_STATIC_INSTANCED, // instanced?
                meshType == MeshType::MESH_TYPE_SKINNED, // skinned?
                shadowPass, // shadow pipeline?
                usedVertexBufferLayouts
            );
            // NOTE: WARNING! There's an issue if the pipeline requires more descriptor set layouts than
            // provided with the inputted uniformResourceLayouts!
            std::vector<DescriptorSetLayout> usedDescriptorSetLayouts;
            for (const ShaderResourceLayout& resourceLayout : uniformResourceLayouts)
                usedDescriptorSetLayouts.push_back(resourceLayout.descriptorSetLayout);

            // NOTE: Don't know if working, not tested!
            std::string vertexShaderFilename = get_shadowpass_shader_name(ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, meshType);
            std::string fragmentShaderFilename = get_shadowpass_shader_name(ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, meshType);
            BatchPipelineData* pPipelineData = createBatchPipelineData(
                pRenderPass,
                vertexShaderFilename,
                fragmentShaderFilename,
                usedVertexBufferLayouts,
                usedDescriptorSetLayouts,
                CullMode::CULL_MODE_FRONT,
                pushConstantsSize,
                pushConstantsShaderStage
            );
            pPipeline = pPipelineData->pPipeline;
        }

        Batch* pBatch = new Batch{
            maxBatchLength,
            pPipeline, // TODO: create pipeline if not getting it from Material!
            combinedDescriptorSets,
            dynamicUniformBufferElementSize,
            { pMesh->getVertexBuffer() }, // Static vertex buffers
            dynamicVertexBuffers,
            pMesh->getIndexBuffer(),
            pushConstantsSize,
            pushConstantsUniformInfos,
            pPushConstantsData,
            // Atm push constants should ONLY be used in vertex shaders!
            pushConstantsShaderStage,
            0, // repeat count
            maxRepeatCount, // max repeat count
            repeatAdvance, // repeat advance
            0, // instance count
            maxInstanceCount, // max instance count
            instanceAdvance // instance advance
        };

        _identifierBatchMapping[renderPassType][batchID] = _batches[renderPassType].size();
        _batches[renderPassType].push_back(pBatch);
    }

    void Batcher::createBatch(
        ID_t meshID,
        ID_t materialID,
        const Light * const pDirectionalLight,
        const RenderPass* pRenderPass
    )
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        if (!pMesh)
        {
            Debug::log(
                "@Batcher::submit "
                "Mesh with ID: " + std::to_string(meshID) + " not found!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::unordered_map<MeshType, BatchTemplate>::iterator templateIt = _batchTemplates.find(pMesh->getType());
        if (templateIt == _batchTemplates.end())
        {
            Debug::log(
                "@Batcher::submit "
                "No batch construction template found for: " + mesh_type_to_string(pMesh->getType()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        const BatchTemplate& creationTemplate = templateIt->second;
        createBatch(
            meshID,
            materialID,
            creationTemplate.maxBatchLength,
            creationTemplate.maxRepeatCount,
            creationTemplate.repeatAdvance,
            creationTemplate.maxInstanceCount,
            creationTemplate.instanceAdvance,
            creationTemplate.instanceBufferElementSize,
            creationTemplate.uniformResourceLayouts,
            pDirectionalLight,
            pRenderPass
        );
    }

    void Batcher::addToBatch(
        ID_t batchID,
        void* pData,
        size_t dataSize,
        const std::vector<size_t>& dataElementSizes,
        size_t currentFrame
    )
    {
        // Need to update each render passes batches' instance or/and repeat counts
        // BUT update their shared transforms buffer ONLY ONCE!
        // ...this is quite dumb I know...
        bool resourcesUpdated = false;
        for (Batch* pBatch : getBatches(batchID))
        {
            // TODO: allow using both, instance and uniform buffers!
            bool usingInstanceBuffer = pBatch->instanceAdvance > 0;

            // TODO: Create new batch if this one is full!
            //  -> Need to have some kind of thing where multiple batches can exist for the same
            //  identifier for the same render pass
            if (pBatch->repeatCount >= pBatch->maxRepeatCount && pBatch->repeatAdvance > 0)
            {
                Debug::log(
                    "@Batcher::addToBatch "
                    "BatchID: " + std::to_string(batchID) + " "
                    "repeat count(" + std::to_string(pBatch->repeatCount) + ") "
                    "reached its maximum(" + std::to_string(pBatch->maxRepeatCount) + ")",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            if (pBatch->instanceCount >= pBatch->maxInstanceCount && pBatch->instanceAdvance > 0)
            {
                Debug::log(
                    "@Batcher::addToBatch "
                    "BatchID: " + std::to_string(batchID) + " "
                    "instance count(" + std::to_string(pBatch->instanceCount) + ") "
                    "reached its maximum(" + std::to_string(pBatch->maxInstanceCount) + ")",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            if (!resourcesUpdated)
            {
                if (batchResourcesExist(batchID))
                {
                    size_t inputDataIndex = 0;
                    size_t inputDataOffset = 0;
                    const uint32_t entryCount = usingInstanceBuffer ? pBatch->instanceCount : pBatch->repeatCount;
                    for (BatchShaderResource& resource : accessSharedBatchResources(batchID))
                    {
                        if (resource.buffer.empty())
                            continue;

                        Buffer* pBuffer = resource.buffer[currentFrame];


                        // FUCKED UP ATM! NEED TO ADVANCE INPUT BUFFER AT DIFFERENT PACE THAN
                        // THE RESOURCE BUFFER, SINCE RESOURCE BUFFER'S ELEM SIZE CAN BE BIGGER
                        // THAN THE ACTUAL DATA ELEM SIZE (dyamic uniform buffer offset alignment
                        // requirement)!!!
                        const size_t bufferUpdateSize = pBuffer->getDataElemSize();
                        const size_t bufferUpdateOffset = pBuffer->getDataElemSize() * entryCount;

                        // Make sure inside input data range
                        if (inputDataIndex > dataElementSizes.size())
                        {
                            Debug::log(
                                "@Batcher::addToBatch "
                                "inputDataIndex(" + std::to_string(inputDataIndex) + ") out of bounds! "
                                "Provided data element sizes: " + std::to_string(dataElementSizes.size()),
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            PLATYPUS_ASSERT(false);
                            return;
                        }
                        if (inputDataOffset > dataSize)
                        {
                            Debug::log(
                                "@Batcher::addToBatch "
                                "inputDataOffset(" + std::to_string(inputDataOffset) + ") out of bounds! "
                                "Inputted data size: " + std::to_string(dataSize),
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            PLATYPUS_ASSERT(false);
                            return;
                        }

                        // Make sure inside resource range
                        if (bufferUpdateSize + bufferUpdateOffset > pBuffer->getTotalSize())
                        {
                            Debug::log(
                                "@Batcher::addToBatch "
                                "buffer updateOffset(" + std::to_string(bufferUpdateOffset) + ") "
                                "out of bounds. Buffer's total size: " + std::to_string(pBuffer->getTotalSize()) + " "
                                "buffer element size: " + std::to_string(pBuffer->getDataElemSize()),
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            PLATYPUS_ASSERT(false);
                            return;
                        }

                        pBuffer->updateHost(
                            (void*)((PE_ubyte*)pData + inputDataOffset),
                            bufferUpdateSize,
                            bufferUpdateOffset
                        );
                        resource.requiresDeviceUpdate = true;
                        inputDataOffset += dataElementSizes[inputDataIndex];
                        ++inputDataIndex;
                    }
                    resourcesUpdated = true;
                }
            }

            if (pBatch->repeatAdvance == 0)
                pBatch->repeatCount = 1;
            else
                pBatch->repeatCount += pBatch->repeatAdvance;

            if (pBatch->instanceAdvance == 0)
                pBatch->instanceCount = 1;
            else
                pBatch->instanceCount += pBatch->instanceAdvance;
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

    std::vector<Buffer*> Batcher::getOrCreateSharedInstancedBuffer(
        ID_t batchID,
        size_t elementSize,
        size_t maxBatchLength,
        size_t framesInFlight
    )
    {
        BatchShaderResource* pBatchResource = getSharedBatchResource(batchID, 0);
        if (pBatchResource)
        {
            Debug::log(
                "@Batcher::getOrCreateSharedInstancedBuffer "
                "Using existing instanced buffer!"
            );
            return pBatchResource->buffer;
        }
        Debug::log(
            "@Batcher::getOrCreateSharedInstancedBuffer "
            "Creating new instanced buffer!"
        );
        // TODO: Make this less dumb -> buffers after creation are available in pBatchResource?
        // ...confusing as fuck...
        std::vector<Buffer*> buffers(framesInFlight);
        createSharedBatchInstancedBuffers(
            batchID,
            elementSize,
            maxBatchLength,
            framesInFlight,
            buffers
        );

        return buffers;
    }

    std::vector<BatchShaderResource>& Batcher::getOrCreateSharedShaderResources(
        ID_t batchID,
        size_t maxBatchLength,
        const std::vector<ShaderResourceLayout>& resourceLayouts,
        size_t framesInFlight
    )
    {
        if (!batchResourcesExist(batchID))
        {
            // TODO: remove outResources from createBatchShaderResources when this works!
            std::vector<BatchShaderResource> shaderResources(resourceLayouts.size());
            createBatchShaderResources(
                framesInFlight,
                batchID,
                maxBatchLength,
                resourceLayouts,
                shaderResources
            );
        }

        return accessSharedBatchResources(batchID);
    }
}
