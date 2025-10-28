#include "TerrainBatch.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"


namespace platypus
{
    // TODO: Optimize, improve!
    //  -> Dumb, dereferencing existing shader resource but returning new by value...
    //      -> Make more coherent!
    static std::vector<BatchShaderResource> get_or_create_terrain_shader_resources(
        Batcher& batcher,
        ID_t identifier,
        const DescriptorSetLayout& descriptorSetLayout,
        size_t maxBatchLength,
        size_t dynamicUniformBufferElementSize,
        size_t framesInFlight
    )
    {
        BatchShaderResource* pBatchResource = batcher.getSharedBatchResource(identifier, 0);
        if (pBatchResource)
        {
            Debug::log(
                "@get_or_create_terrain_shader_resources "
                "Using existing transforms buffer!"
            );
            return { *pBatchResource }; // dumb...
        }
        Debug::log(
            "@get_or_create_terrain_shader_resources "
            "Creating new transforms buffer!"
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
                    descriptorSetLayout,
                    { }
                }
            },
            shaderResources
        );

        return shaderResources;
    }

    Batch* create_terrain_batch(
        Batcher& batcher,
        size_t maxLength,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID
    )
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!batcher.validateBatchDoesntExist("create_terrain_batch", pRenderPass->getType(), identifier))
            return nullptr;

        Batch* pBatch = new Batch;
        pBatch->maxLength = maxLength;

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        pBatch->pPipeline = pMaterial->getPipeline(ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE);

        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        pBatch->dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f)
        );
        std::vector<BatchShaderResource> shaderResources = get_or_create_terrain_shader_resources(
            batcher,
            identifier,
            Batcher::get_terrain_descriptor_set_layout(),
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

        return pBatch;
    }

    void add_to_terrain_batch(
        Batcher& batcher,
        ID_t batchID,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    )
    {
        // Need to update each render passes batches' instance or/and repeat counts
        // BUT update their shared resources ONLY ONCE!
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
                    "@add_to_terrain_batch "
                    "Batch is full. Maximum terrain batch length is " + std::to_string(pBatch->maxLength),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            // TODO: Remove tile size and vertices per row from terrain shader resources data!
            if (!sharedBufferUpdated)
            {
                batcher.updateHostSideSharedResource(
                    batchID,
                    0,
                    (void*)&transformationMatrix,
                    sizeof(Matrix4f),
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
