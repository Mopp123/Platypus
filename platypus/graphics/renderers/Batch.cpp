#include "Batch.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    std::vector<Batch*> BatchPool::s_batches;
    std::unordered_map<ID_t, size_t> BatchPool::s_identifierBatchMapping;
    size_t BatchPool::s_maxStaticBatchLength = 10000;
    std::vector<Buffer*> BatchPool::s_allocatedBuffers;
    std::unordered_map<ID_t, size_t> BatchPool::s_batchAdditionalBufferMapping;

    Batch* BatchPool::get_batch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);

        std::unordered_map<ID_t, size_t>::const_iterator indexIt = s_identifierBatchMapping.find(identifier);
        if (indexIt == s_identifierBatchMapping.end())
            return nullptr;

        // NOTE: Is this safe enough? Probably not...
        return s_batches[indexIt->second];
    }

    // NOTE: Could even create the pipelines dynamically here, so wouldn't require having those inside
    // Material anymore?
    Batch* BatchPool::create_static_batch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (s_identifierBatchMapping.find(identifier) != s_identifierBatchMapping.end())
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (s_batchAdditionalBufferMapping.find(identifier) != s_batchAdditionalBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Instanced vertex buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

        BatchType type = BatchType::STATIC_INSTANCED;
        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        size_t descriptorSetCountPerFrame = pMasterRenderer->getScene3DDataDescriptorSets().size();
        if (pMaterial->getDescriptorSets().size() != descriptorSetCountPerFrame)
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Mismatch in descriptor set counts for frames! "
                "Common descriptor set count: " + std::to_string(descriptorSetCountPerFrame) + " "
                "Material descriptor set count: " + std::to_string(pMaterial->getDescriptorSets().size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        std::vector<std::vector<DescriptorSet>> allDescriptorSets(descriptorSetCountPerFrame);
        for (size_t i = 0; i < descriptorSetCountPerFrame; ++i)
        {
            allDescriptorSets[i] = {
                pMasterRenderer->getScene3DDataDescriptorSets()[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        std::vector<uint32_t> dynamicDescriptorSetRanges = { };

        // Create the instanced transforms buffer
        // NOTE: Should these be created separately for each frame in flight as well?
        std::vector<Matrix4f> transformsBuffer(s_maxStaticBatchLength);
        Buffer* pInstancedBuffer = new Buffer(
            transformsBuffer.data(),
            sizeof(Matrix4f),
            transformsBuffer.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM,
            true
        );
        s_batchAdditionalBufferMapping[identifier] = s_allocatedBuffers.size();
        s_allocatedBuffers.push_back(pInstancedBuffer);
        std::vector<const Buffer*> vertexBuffers = {
            pMesh->getVertexBuffer(),
            pInstancedBuffer
        };

        Batch* pBatch = new Batch{
            type,
            pPipeline,
            allDescriptorSets,
            { }, // dynamic descriptor set ranges
            vertexBuffers,
            pMesh->getIndexBuffer(),
            0, // push constants size
            { }, // push constant uniform infos
            nullptr, // push constants data
            ShaderStageFlagBits::SHADER_STAGE_NONE, // push constants shader stage flags
            1, // repeat count
            0 // instance count
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        s_identifierBatchMapping[identifier] = s_batches.size();
        s_batches.push_back(pBatch);
        return pBatch;
    }

    void BatchPool::add_to_static_batch(Batch* pBatch, ID_t identifier, const Matrix4f& transformationMatrix)
    {
        std::unordered_map<ID_t, size_t>::const_iterator additionalBufferIndexIt = s_batchAdditionalBufferMapping.find(identifier);
        if (additionalBufferIndexIt == s_batchAdditionalBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Failed to find instanced buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->instanceCount >= s_maxStaticBatchLength)
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Batch is full. Maximum static batch length is " + std::to_string(s_maxStaticBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        s_allocatedBuffers[additionalBufferIndexIt->second]->updateHost(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            sizeof(Matrix4f) * pBatch->instanceCount
        );
        ++pBatch->instanceCount;
        pBatch->repeatCount = 1;
    }

    void BatchPool::update_device_side_buffers()
    {
        for (Buffer* pBuffer : s_allocatedBuffers)
        {
            pBuffer->updateDevice(
                pBuffer->accessData(),
                pBuffer->getTotalSize(),
                0
            );
        }
    }

    void BatchPool::reset_for_next_frame()
    {
        for (Batch* pBatch : s_batches)
        {
            pBatch->repeatCount = 0;
            pBatch->instanceCount = 0;
        }
    }

    void BatchPool::free_batches()
    {
        for (Batch* pBatch : s_batches)
            delete pBatch;

        s_batches.clear();
        s_identifierBatchMapping.clear();
        s_batchAdditionalBufferMapping.clear();

        for (Buffer* pBuffer : s_allocatedBuffers)
            delete pBuffer;

        s_allocatedBuffers.clear();
    }

    const std::vector<Batch*>& BatchPool::get_batches()
    {
        return s_batches;
    }
}
