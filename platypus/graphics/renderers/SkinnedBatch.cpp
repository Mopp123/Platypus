#include "SkinnedBatch.hpp"
#include "platypus/core/Application.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.h"


namespace platypus
{
    // TODO: Optimize, improve!
    //  -> Dumb, dereferencing existing shader resource but returning new by value...
    //      -> Make more coherent!
    static std::vector<BatchShaderResource> get_or_create_joint_buffer(
        Batcher& batcher,
        ID_t identifier,
        const DescriptorSetLayout& jointDescriptorSetLayout,
        size_t maxBatchLength,
        size_t dynamicUniformBufferElementSize,
        size_t framesInFlight
    )
    {
        BatchShaderResource* pBatchResource = batcher.getSharedBatchResource(identifier, 0);
        if (pBatchResource)
        {
            Debug::log(
                "@get_or_create_joint_buffer "
                "Using existing joint buffer!"
            );
            return { *pBatchResource }; // dumb...
        }
        Debug::log(
            "@get_or_create_joint_buffer "
            "Creating new joint buffer!"
        );

        // TODO: Make this less dumb -> buffers after creation are available in pBatchResource?
        // ...confusing as fuck...
        std::vector<BatchShaderResource> shaderResources(1);
        batcher.createBatchShaderResources(
            framesInFlight,
            identifier,
            maxBatchLength,
            {
                {
                    ShaderResourceType::ANY,
                    dynamicUniformBufferElementSize,
                    jointDescriptorSetLayout,
                    { }
                }
            },
            shaderResources
        );

        return shaderResources;
    }

    Batch* create_skinned_batch(
        Batcher& batcher,
        size_t maxLength,
        size_t maxJoints,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID,
        const Light * const pDirectionalLight
    )
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!batcher.validateBatchDoesntExist("create_skinned_batch", pRenderPass->getType(), identifier))
            return nullptr;

        Batch* pBatch = new Batch;
        pBatch->maxLength = maxLength;

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        pBatch->pPipeline = pMaterial->getPipeline(ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE);

        if (pMaterial->receivesShadows())
        {
            if (!pDirectionalLight)
            {
                Debug::log(
                    "@create_skinned_batch "
                    "Directional light was nullptr!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                delete pBatch;
                return nullptr;
            }
            // TODO: Better way of handling this
            pBatch->pushConstantsSize = sizeof(Matrix4f) * 2;
            pBatch->pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
            pBatch->pushConstantsUniformInfos = {
                { ShaderDataType::Mat4 },
                { ShaderDataType::Mat4 }
            };
            pBatch->pPushConstantsData = (void*)pDirectionalLight;
        }

        // Get or create joint buffer
        const size_t dynamicUboElemSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * maxJoints
        );
        pBatch->dynamicUniformBufferElementSize = dynamicUboElemSize;
        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        std::vector<BatchShaderResource> shaderResources = get_or_create_joint_buffer(
            batcher,
            identifier,
            Batcher::get_joint_descriptor_set_layout(),
            pBatch->maxLength,
            pBatch->dynamicUniformBufferElementSize,
            framesInFlight
        );

        pBatch->descriptorSets = batcher.combineUsedDescriptorSets(
            framesInFlight,
            {
                pMasterRenderer->getScene3DDataDescriptorSets(),
                shaderResources[0].descriptorSet,
                pMaterial->getDescriptorSets()
            }
        );

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        pBatch->staticVertexBuffers = { pMesh->getVertexBuffer() };
        pBatch->pIndexBuffer = pMesh->getIndexBuffer();

        batcher.addBatch(pRenderPass->getType(), identifier, pBatch);

        // NOTE: Unnecessary to actually return pBatch here!
        return pBatch;
    }

    Batch* create_skinned_shadow_batch(
        Batcher& batcher,
        size_t maxLength,
        size_t maxJoints,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID,
        void* pShadowPushConstants,
        size_t shadowPushConstantsSize
    )
    {
        // TODO: Some better way to deal with these...
        const size_t requiredShadowPushConstantsSize = sizeof(Matrix4f) * 2;
        if (pShadowPushConstants != nullptr && shadowPushConstantsSize != requiredShadowPushConstantsSize)
        {
            Debug::log(
                "@create_skinned_shadow_batch "
                "Invalid shadow push constants size: " + std::to_string(shadowPushConstantsSize) + " "
                "required size is " + std::to_string(requiredShadowPushConstantsSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        ID_t identifier = ID::hash(meshID, materialID);
        if (!batcher.validateBatchDoesntExist("create_skinned_shadow_batch", pRenderPass->getType(), identifier))
            return nullptr;

        Batch* pBatch = new Batch;
        pBatch->maxLength = maxLength;

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        // Get or create joint buffer
        pBatch->dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * maxJoints
        );
        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        std::vector<BatchShaderResource> shaderResources = get_or_create_joint_buffer(
            batcher,
            identifier,
            Batcher::get_joint_descriptor_set_layout(),
            pBatch->maxLength,
            pBatch->dynamicUniformBufferElementSize,
            framesInFlight
        );

        pBatch->descriptorSets = batcher.combineUsedDescriptorSets(
            framesInFlight,
            {
                shaderResources[0].descriptorSet
            }
        );

        pBatch->pushConstantsSize = shadowPushConstantsSize;
        pBatch->pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
        pBatch->pushConstantsUniformInfos = {
            { ShaderDataType::Mat4 },
            { ShaderDataType::Mat4 }
        };
        pBatch->pPushConstantsData = pShadowPushConstants;

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        pBatch->staticVertexBuffers = { pMesh->getVertexBuffer() };
        pBatch->pIndexBuffer = pMesh->getIndexBuffer();

        std::vector<VertexBufferLayout> usedVertexBufferLayouts;
        pMasterRenderer->solveVertexBufferLayouts(
            pMesh->getVertexBufferLayout(),
            false,
            true,
            true,
            usedVertexBufferLayouts
        );
        BatchPipelineData* pPipelineData = batcher.createBatchPipelineData(
            pRenderPass,
            "shadows/SkinnedVertexShader",
            "shadows/SkinnedFragmentShader",
            usedVertexBufferLayouts,
            { Batcher::get_joint_descriptor_set_layout() },
            pBatch->pushConstantsSize,
            pBatch->pushConstantsShaderStage
        );
        pBatch->pPipeline = pPipelineData->pPipeline;

        batcher.addBatch(pRenderPass->getType(), identifier, pBatch);

        // NOTE: Unnecessary to actually return pBatch here!
        return pBatch;
    }

    void add_to_skinned_batch(
        Batcher& batcher,
        ID_t batchID,
        void* pJointData,
        size_t jointDataSize,
        size_t currentFrame
    )
    {
        // Need to update each render passes batches' instance or/and repeat counts
        // BUT update their shared transforms buffer ONLY ONCE!
        // ...this is quite dumb I know...
        bool sharedBufferUpdated = false;
        for (Batch* pBatch : batcher.getBatches(batchID))
        {
            // TODO: Create new batch if this one is full!
            //  -> Need to have some kind of thing where multiple batches can exist for the same
            //  identifier for the same render pass
            if (pBatch->instanceCount >= pBatch->maxLength)
            {
                Debug::log(
                    "@add_to_skinned_batch "
                    "Batch is full. Maximum skinned batch length is " + std::to_string(pBatch->maxLength),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            if (!sharedBufferUpdated)
            {
                batcher.updateHostSideSharedResource(
                    batchID,
                    0,
                    pJointData,
                    jointDataSize,
                    pBatch->dynamicUniformBufferElementSize * pBatch->repeatCount,
                    currentFrame
                );
                sharedBufferUpdated = true;
            }

            pBatch->instanceCount = 1;
            ++pBatch->repeatCount;
        }
    }
}
