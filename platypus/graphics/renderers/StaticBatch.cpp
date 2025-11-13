#include "StaticBatch.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"


namespace platypus
{
    static std::vector<Buffer*> get_or_create_transforms_buffer(
        Batcher& batcher,
        ID_t identifier,
        size_t maxBatchLength,
        size_t framesInFlight
    )
    {
        BatchShaderResource* pBatchResource = batcher.getSharedBatchResource(identifier, 0);
        if (pBatchResource)
        {
            Debug::log(
                "@get_or_create_transforms_buffer(StaticBatch) "
                "Using existing transforms buffer!"
            );
            return pBatchResource->buffer;
        }
        Debug::log(
            "@get_or_create_transforms_buffer(StaticBatch) "
            "Creating new transforms buffer!"
        );
        // TODO: Make this less dumb -> buffers after creation are available in pBatchResource?
        // ...confusing as fuck...
        std::vector<Buffer*> buffers(framesInFlight);
        batcher.createSharedBatchInstancedBuffers(
            identifier,
            sizeof(Matrix4f),
            maxBatchLength,
            framesInFlight,
            buffers
        );

        return buffers;
    }

    Batch* create_static_batch(
        Batcher& batcher,
        size_t maxLength,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID,
        const Light * const pDirectionalLight
    )
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!batcher.validateBatchDoesntExist("create_static_batch", pRenderPass->getType(), identifier))
            return nullptr;

        Batch* pBatch = new Batch;
        pBatch->maxLength = maxLength;

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        pBatch->pPipeline = pMaterial->getPipeline(ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE);

        if (pMaterial->receivesShadows())
        {
            if (!pDirectionalLight)
            {
                Debug::log(
                    "@create_static_batch "
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

        // TODO: Validate descriptor set counts against frames in flight and each others
        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        pBatch->descriptorSets = batcher.combineUsedDescriptorSets(
            framesInFlight,
            {
                pMasterRenderer->getScene3DDataDescriptorSets(),
                pMaterial->getDescriptorSets()
            }
        );

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        pBatch->staticVertexBuffers = { pMesh->getVertexBuffer() };

        // One dynamic vertex buffer for each frame in flight
        pBatch->dynamicVertexBuffers.resize(1);
        pBatch->dynamicVertexBuffers[0] = get_or_create_transforms_buffer(
            batcher,
            identifier,
            pBatch->maxLength,
            framesInFlight
        );
        pBatch->pIndexBuffer = pMesh->getIndexBuffer();

        batcher.addBatch(pRenderPass->getType(), identifier, pBatch);

        return pBatch;
    }

    Batch* create_static_shadow_batch(
        Batcher& batcher,
        size_t maxLength,
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
                "@create_static_shadow_batch "
                "Invalid shadow push constants size: " + std::to_string(shadowPushConstantsSize) + " "
                "required size is " + std::to_string(requiredShadowPushConstantsSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        ID_t identifier = ID::hash(meshID, materialID);
        if (!batcher.validateBatchDoesntExist("create_static_shadow_batch", pRenderPass->getType(), identifier))
            return nullptr;

        Batch* pBatch = new Batch;
        pBatch->maxLength = maxLength;

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        // TODO: Validate descriptor set counts against frames in flight and each others
        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        pBatch->staticVertexBuffers = { pMesh->getVertexBuffer() };

        // One dynamic vertex buffer for each frame in flight
        pBatch->dynamicVertexBuffers.resize(1);
        pBatch->dynamicVertexBuffers[0] = get_or_create_transforms_buffer(
            batcher,
            identifier,
            pBatch->maxLength,
            framesInFlight
        );
        pBatch->pIndexBuffer = pMesh->getIndexBuffer();

        pBatch->pushConstantsSize = shadowPushConstantsSize;
        pBatch->pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
        pBatch->pushConstantsUniformInfos = {
            { ShaderDataType::Mat4 },
            { ShaderDataType::Mat4 }
        };
        pBatch->pPushConstantsData = pShadowPushConstants;

        std::vector<VertexBufferLayout> usedVertexBufferLayouts;
        pMasterRenderer->solveVertexBufferLayouts(
            pMesh->getVertexBufferLayout(),
            true,
            false,
            true,
            usedVertexBufferLayouts
        );
        BatchPipelineData* pPipelineData = batcher.createBatchPipelineData(
            pRenderPass,
            "shadows/StaticVertexShader",
            "shadows/StaticFragmentShader",
            usedVertexBufferLayouts,
            { }, // DescriptorSetLayouts
            pBatch->pushConstantsSize,
            pBatch->pushConstantsShaderStage
        );
        pBatch->pPipeline = pPipelineData->pPipeline;

        batcher.addBatch(pRenderPass->getType(), identifier, pBatch);
        return pBatch;
    }

    void add_to_static_batch(
        Batcher& batcher,
        ID_t batchID,
        const Matrix4f& transformationMatrix,
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
                    "@add_to_static_batch "
                    "Batch is full. Maximum static batch length is " + std::to_string(pBatch->maxLength),
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
                    (void*)&transformationMatrix,
                    sizeof(Matrix4f),
                    sizeof(Matrix4f) * pBatch->instanceCount,
                    currentFrame
                );

                sharedBufferUpdated = true;
            }

            ++pBatch->instanceCount;
            pBatch->repeatCount = 1;
        }
    }
}
