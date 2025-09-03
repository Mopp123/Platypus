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
        std::vector<CommandBuffer> _commandBuffers;
        DescriptorPool& _descriptorPoolRef;

        size_t _currentFrame = 0;
        uint64_t _requiredComponentsMask = 0;

    public:
        Renderer(
            const MasterRenderer& masterRenderer,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        virtual ~Renderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        virtual void createDescriptorSets() {}
        virtual void freeDescriptorSets() {}

        // NOTE: Should rather
        virtual void freeBatches() = 0;
        virtual void submit(const Scene* pScene, entityID_t entity) = 0;

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const DescriptorSet& commonDescriptorSet,
            size_t frame
        ) = 0;

        inline uint64_t getRequiredComponentsMask() const { return _requiredComponentsMask; }
    };
}
