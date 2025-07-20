#include "SkinnedMeshRenderer.hpp"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cstring>
#include <cmath>


namespace platypus
{
    size_t SkinnedMeshRenderer::s_maxJoints = 55;
    SkinnedMeshRenderer::SkinnedMeshRenderer(
        const MasterRenderer& masterRenderer,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool,
        uint64_t requiredComponentsMask
    ) :
        Renderer(
            masterRenderer,
            commandPool,
            descriptorPool,
            requiredComponentsMask
        ),
        _jointDescriptorSetLayout(
        {
            {
                0,
                1,
                DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, // NOTE: Should probably be dynamix uniform buffer...
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                { { 0, ShaderDataType::Mat4 } }
            }
        }
        )
    {
        // NOTE: How to recreate descriptor sets?
        //  -> should the inherited Renderer have some overrideable funcs for that?
    }

    SkinnedMeshRenderer::~SkinnedMeshRenderer()
    {
    }

    void SkinnedMeshRenderer::freeBatches()
    {
    }

    void SkinnedMeshRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        const SkinnedMeshRenderable* pRenderable = (const SkinnedMeshRenderable*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE
        );
        const Transform* pTransform = (const Transform*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        const Matrix4f transformationMatrix = pTransform->globalMatrix;

        ID_t meshID = pRenderable->meshID;
        ID_t materialID = pRenderable->materialID;

        _renderData.push_back({ meshID, materialID, transformationMatrix });
    }

    const CommandBuffer& SkinnedMeshRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& perspectiveProjectionMatrix,
        const Matrix4f& orthographicProjectionMatrix,
        const DescriptorSet& cameraDescriptorSet,
        const DescriptorSet& dirLightDescriptorSet,
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

            render::bind_pipeline(
                currentCommandBuffer,
                pMaterial->getSkinnedPipelineData()->pipeline
            );

            Matrix4f pushConstants[1] = { perspectiveProjectionMatrix };
            render::push_constants(
                currentCommandBuffer,
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(Matrix4f),
                pushConstants,
                {
                    { 0, ShaderDataType::Mat4 }
                }
            );

            render::bind_vertex_buffers(
                currentCommandBuffer,
                {
                    pMesh->getVertexBuffer()
                }
            );
            const Buffer* pIndexBuffer = pMesh->getIndexBuffer();
            render::bind_index_buffer(currentCommandBuffer, pIndexBuffer);

            std::vector<DescriptorSet> descriptorSetsToBind = {
                cameraDescriptorSet,
                dirLightDescriptorSet,
                pMaterial->getDescriptorSets()[_currentFrame]
            };

            render::bind_descriptor_sets(
                currentCommandBuffer,
                descriptorSetsToBind,
                { }
            );

            render::draw_indexed(
                currentCommandBuffer,
                (uint32_t)pIndexBuffer->getDataLength(),
                1
            );
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
}
