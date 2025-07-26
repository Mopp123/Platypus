#include "SkinnedMeshRenderer.hpp"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/RenderCommand.h"
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
            },
            {
                1,
                1,
                DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, // NOTE: Should probably be dynamix uniform buffer...
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                { { 0, ShaderDataType::Mat4 } }
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
        for (Buffer* pInverseBuffer : _inverseBindMatricesBuffer)
            delete pInverseBuffer;

        _jointUniformBuffer.clear();
        _inverseBindMatricesBuffer.clear();
    }

    void SkinnedMeshRenderer::createDescriptorSets()
    {
        std::vector<Matrix4f> jointBufferData(s_maxJoints);
        memset(jointBufferData.data(), 0, sizeof(Matrix4f) * jointBufferData.size());

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            Buffer* pJointUniformBuffer = new Buffer(
                _commandPoolRef,
                jointBufferData.data(),
                sizeof(Matrix4f),
                jointBufferData.size(),
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _jointUniformBuffer.push_back(pJointUniformBuffer);

            Buffer* pInverseBindMatrixBuffer = new Buffer(
                _commandPoolRef,
                jointBufferData.data(),
                sizeof(Matrix4f),
                jointBufferData.size(),
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _inverseBindMatricesBuffer.push_back(pInverseBindMatrixBuffer);

            _jointDescriptorSet.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    &_jointDescriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pJointUniformBuffer },
                        { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pInverseBindMatrixBuffer },
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
        for (Buffer* pBuffer : _inverseBindMatricesBuffer)
            delete pBuffer;

        _inverseBindMatricesBuffer.clear();
        _jointUniformBuffer.clear();
    }

    void SkinnedMeshRenderer::freeBatches()
    {
    }

    void SkinnedMeshRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        const SkinnedMeshRenderable* pRenderable = (const SkinnedMeshRenderable*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE
        );
        const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
        );
        const Transform* pTransform = (const Transform*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        const Matrix4f transformationMatrix = pTransform->globalMatrix;

        ID_t meshID = pRenderable->meshID;
        ID_t materialID = pRenderable->materialID;

        SkeletalAnimationData* pAnimAsset = (SkeletalAnimationData*)Application::get_instance()->getAssetManager()->getAsset(
            pAnimation->animationID,
            AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA
        );

        // NOTE: Just testing atm! DANGEROUS AS HELL!!!
        // *Allocated transform skeleton should be able to be accessed like this
        // TODO: Make this safe and faster
        size_t jointCount = pAnimation->jointCount;
        std::vector<Matrix4f> jointMatrices(jointCount);
        std::vector<Matrix4f> inverseBindMatrices(jointCount);
        for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
        {
            Transform* pJointTransform = (Transform*)pScene->getComponent(
                entity + jointIndex,
                ComponentType::COMPONENT_TYPE_TRANSFORM
            );
            //pJointTransform->globalMatrix = resultMatrix;
            jointMatrices[jointIndex] = pJointTransform->globalMatrix;
            inverseBindMatrices[jointIndex] = pAnimAsset->getBindPose().joints[jointIndex].inverseMatrix;
        }



        _renderData.push_back({ meshID, materialID, jointMatrices, inverseBindMatrices });
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

            _jointUniformBuffer[_currentFrame]->updateDeviceAndHost(
                (void*)renderData.jointMatrices.data(),
                sizeof(Matrix4f) * renderData.jointMatrices.size(),
                0
            );
            _inverseBindMatricesBuffer[_currentFrame]->updateDeviceAndHost(
                (void*)renderData.inverseMatrices.data(),
                sizeof(Matrix4f) * renderData.inverseMatrices.size(),
                0
            );

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
                _jointDescriptorSet[_currentFrame],
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
