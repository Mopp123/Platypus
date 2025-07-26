#pragma once

#include "Renderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/assets/Mesh.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Lights.h"
#include <cstdlib>


namespace platypus
{
    class SkinnedMeshRenderer : public Renderer
    {
    private:
        struct RenderData
        {
            ID_t meshID = NULL_ID;
            ID_t materialID = NULL_ID;
            std::vector<Matrix4f> jointMatrices;
            std::vector<Matrix4f> inverseMatrices;
        };

        static size_t s_maxJoints;

        std::vector<Buffer*> _jointUniformBuffer;
        std::vector<Buffer*> _inverseBindMatricesBuffer;
        DescriptorSetLayout _jointDescriptorSetLayout;
        std::vector<DescriptorSet> _jointDescriptorSet;
        std::vector<RenderData> _renderData;

    public:
        SkinnedMeshRenderer(
            const MasterRenderer& masterRenderer,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        ~SkinnedMeshRenderer();

        virtual void createDescriptorSets() override;
        virtual void freeDescriptorSets() override;

        virtual void freeBatches();
        virtual void submit(const Scene* pScene, entityID_t entity);

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& perspectiveProjectionMatrix,
            const Matrix4f& orthographicProjectionMatrix,
            const DescriptorSet& cameraDescriptorSet,
            const DescriptorSet& dirLightDescriptorSet,
            size_t frame
        );

        inline const DescriptorSetLayout& getDescriptorSetLayout() const { return _jointDescriptorSetLayout; }
        static size_t get_max_joints();
    };
}
