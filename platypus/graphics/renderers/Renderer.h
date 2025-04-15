#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Pipeline.h"
#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include <cstdlib>


namespace platypus
{
    class MasterRenderer;
    class Renderer
    {
    protected:
        const MasterRenderer& _masterRendererRef;
        CommandPool& _commandPoolRef;
        std::vector<CommandBuffer> _commandBuffers;
        DescriptorPool& _descriptorPoolRef;
        Pipeline _pipeline;

        size_t _currentFrame = 0;

        uint32_t _requiredComponentsMask = 0;

    public:
        Renderer(
            const MasterRenderer& masterRenderer,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool,
            uint32_t requiredComponentsMask
        );
        virtual ~Renderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        virtual void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const DescriptorSetLayout& dirLightDescriptorSetLayout
        ) = 0;
        void destroyPipeline();

        virtual void submit(const Scene* pScene, entityID_t entity) = 0;

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& projectionMatrix,
            const Matrix4f& viewMatrix,
            const DescriptorSet& dirLightDescriptorSet,
            size_t frame
        ) = 0;

        inline uint32_t getRequiredComponentsMask() const { return _requiredComponentsMask; }
    };
}
