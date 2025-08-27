#include "SkinnedMeshRenderer.hpp"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/core/Application.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cstring>
#include <cmath>


namespace platypus
{
    size_t SkinnedMeshRenderer::s_maxJoints = 50;
    size_t SkinnedMeshRenderer::s_maxRenderables = 1024; // TODO: Make this configurable
    SkinnedMeshRenderer::SkinnedMeshRenderer(
        const MasterRenderer& masterRenderer,
        DescriptorPool& descriptorPool,
        uint64_t requiredComponentsMask
    ) :
        Renderer(
            masterRenderer,
            descriptorPool,
            requiredComponentsMask
        ),
        _jointDescriptorSetLayout(
        {
            {
                0,
                1,
                DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, // NOTE: Should probably be dynamix uniform buffer...
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                { { 5, ShaderDataType::Mat4, (int)s_maxJoints } }
            }
        }
        )
    {
        createDescriptorSets();
    }

    SkinnedMeshRenderer::~SkinnedMeshRenderer()
    {
        freeDescriptorSets();
        _jointDescriptorSetLayout.destroy();

        for (Buffer* pJointBuffer : _jointUniformBuffer)
            delete pJointBuffer;

        _jointUniformBuffer.clear();
    }

    void SkinnedMeshRenderer::createDescriptorSets()
    {
        _uniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * s_maxJoints
        );
        std::vector<char> jointBufferData(_uniformBufferElementSize * s_maxRenderables);

        memset(jointBufferData.data(), 0, jointBufferData.size());

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            Buffer* pJointUniformBuffer = new Buffer(
                jointBufferData.data(),
                _uniformBufferElementSize,
                s_maxRenderables,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _jointUniformBuffer.push_back(pJointUniformBuffer);

            _jointDescriptorSet.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    &_jointDescriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, pJointUniformBuffer }
                    }
                )
            );
        }
    }

    void SkinnedMeshRenderer::freeDescriptorSets()
    {
        _descriptorPoolRef.freeDescriptorSets(_jointDescriptorSet);
        _jointDescriptorSet.clear();

        for (Buffer* pBuffer : _jointUniformBuffer)
            delete pBuffer;

        _jointUniformBuffer.clear();
    }

    void SkinnedMeshRenderer::freeBatches()
    {
    }

    void SkinnedMeshRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        if (_renderData.size() >= s_maxRenderables)
        {
            Debug::log(
                "@SkinnedMeshRenderer::submit "
                "Maximum amount of skinned mesh renderables have already been submitted for rendering! "
                "Current limit: " + std::to_string(s_maxRenderables),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const SkinnedMeshRenderable* pRenderable = (const SkinnedMeshRenderable*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE
        );
        const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
        );

        ID_t meshID = pRenderable->meshID;
        ID_t materialID = pRenderable->materialID;

        size_t jointCount = pAnimation->jointCount;
        std::vector<Matrix4f> jointMatrices(jointCount);
        memcpy(
            (void*)jointMatrices.data(),
            pAnimation->jointMatrices,
            sizeof(Matrix4f) * jointCount
        );

        _renderData.push_back({ meshID, materialID, jointMatrices });
    }

    const CommandBuffer& SkinnedMeshRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const DescriptorSet& commonDescriptorSet,
        size_t frame
    )
    {
        #ifdef PLATYPUS_DEBUG
            if (_currentFrame >= _commandBuffers.size())
            {
                Debug::log(
                    "@SkinnedMeshRenderer::recordCommandBuffer "
                    "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                    "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        size_t renderDataIndex = 0;
        for (const RenderData& renderData : _renderData)
        {
            Mesh* pMesh = (Mesh*)pAssetManager->getAsset(
                renderData.meshID,
                AssetType::ASSET_TYPE_MESH
            );
            Material* pMaterial = (Material*)pAssetManager->getAsset(
                renderData.materialID,
                AssetType::ASSET_TYPE_MATERIAL
            );

            // Make sure valid material
            if (!pMaterial)
            {
                Debug::log(
                    "@SkinnedMeshRenderer::recordCommandBuffer "
                    "Material was nullptr",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            // Make sure descriptor sets has been created
            if (!pMaterial->hasDescriptorSets())
            {
                Debug::log(
                    "@SkinnedMeshRenderer::recordCommandBuffer "
                    "No descriptor sets found for material: " + std::to_string(renderData.materialID),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            uint32_t jointBufferOffset = renderDataIndex * _uniformBufferElementSize;
            _jointUniformBuffer[_currentFrame]->updateDeviceAndHost(
                (void*)(renderData.jointMatrices.data()),
                sizeof(Matrix4f) * renderData.jointMatrices.size(),
                jointBufferOffset
            );

            // DANGER! Might dereference nullptr!
            render::bind_pipeline(
                currentCommandBuffer,
                *pMaterial->getSkinnedPipelineData()->pPipeline
            );

            render::bind_vertex_buffers(
                currentCommandBuffer,
                {
                    pMesh->getVertexBuffer()
                }
            );
            const Buffer* pIndexBuffer = pMesh->getIndexBuffer();
            render::bind_index_buffer(currentCommandBuffer, pIndexBuffer);

            render::bind_descriptor_sets(
                currentCommandBuffer,
                {
                    commonDescriptorSet,
                    _jointDescriptorSet[_currentFrame],
                    pMaterial->getDescriptorSets()[_currentFrame]
                },
                { jointBufferOffset }
            );

            render::draw_indexed(
                currentCommandBuffer,
                (uint32_t)pIndexBuffer->getDataLength(),
                1
            );

            ++renderDataIndex;
        }

        currentCommandBuffer.end();

        _renderData.clear();

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    size_t SkinnedMeshRenderer::get_max_joints()
    {
        return s_maxJoints;
    }

    size_t  SkinnedMeshRenderer::get_max_renderables()
    {
        return s_maxRenderables;
    }
}
