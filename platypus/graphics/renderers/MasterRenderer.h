#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/graphics/Framebuffer.hpp"
#include "GUIRenderer.h"
#include "Renderer3D.hpp"
#include "Batch.hpp"

#include <memory>


namespace platypus
{
    struct Scene3DData
    {
        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f viewMatrix = Matrix4f(1.0f);
        Vector4f cameraPosition = Vector4f(0, 0, 0, 1);

        Vector4f ambientLightColor = Vector4f(0.1f, 0.1f, 0.1f, 1);
        Vector4f lightDirection = Vector4f(0, 0, 0, 0);
        Vector4f lightColor = Vector4f(1, 1, 1, 1);
        // x = shadowmap width, y = pcf sample radius, z = shadow strength, w = undetermined atm
        Vector4f shadowProperties;
    };

    class MasterRenderer
    {
    private:
        Swapchain& _swapchainRef;
        DescriptorPool _descriptorPool;
        Batcher _batcher;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        // Shared descriptor sets among multiple renderers
        Scene3DData _scene3DData;
        std::vector<Buffer*> _scene3DDataUniformBuffers;
        DescriptorSetLayout _scene3DDataDescriptorSetLayout;
        std::vector<DescriptorSet> _scene3DDescriptorSets;

        std::unique_ptr<Renderer3D> _pRenderer3D;
        std::unique_ptr<GUIRenderer> _pGUIRenderer;

        RenderPass _shadowPass;
        // NOTE: Switched using single framebuffer and textures for testing offscreen rendering..
        //  -> This should be fine since these are produced and consumed by GPU and CPU doesn't
        //  touch these + the mem barrier in render commands
        Framebuffer* _pShadowFramebuffer;
        TextureSampler _shadowTextureSampler;
        Texture* _pShadowFramebufferColorTexture;
        Texture* _pShadowFramebufferDepthTexture;
        uint32_t _shadowmapWidth = 2048;

        DescriptorSetLayout _shadowmapDescriptorSetLayout;

        size_t _currentFrame = 0;

    public:
        // NOTE: CommandPool and Device must exist when creating this
        MasterRenderer(Swapchain& swapchain);
        ~MasterRenderer();
        void createPipelines();

        void cleanRenderers();
        void cleanUp();
        void submit(const Scene* pScene, const Entity& entity);
        void render(const Window& window);

        void solveVertexBufferLayouts(
            const VertexBufferLayout& meshVertexBufferLayout,
            bool instanced,
            bool skinned,
            bool shadowPipeline,
            std::vector<VertexBufferLayout>& outVertexBufferLayouts
        ) const;
        void solveDescriptorSetLayouts(
            const Material* pMaterial,
            bool skinned,
            bool shadowPipeline,
            std::vector<DescriptorSetLayout>& outDescriptorSetLayouts
        ) const;

        inline const RenderPass& getShadowPass() const { return _shadowPass; }
        inline Texture* getShadowFramebufferColorTexture() { return _pShadowFramebufferColorTexture; }
        inline Texture* getShadowFramebufferDepthTexture() { return _pShadowFramebufferDepthTexture; }

        inline const Swapchain& getSwapchain() const { return _swapchainRef; }
        inline DescriptorPool& getDescriptorPool() { return _descriptorPool; }

        inline const DescriptorSetLayout& getScene3DDataDescriptorSetLayout() const { return _scene3DDataDescriptorSetLayout; }
        inline const DescriptorSetLayout& getShadowmapDescriptorSetLayout() const { return _shadowmapDescriptorSetLayout; }

        inline const std::vector<DescriptorSet>& getScene3DDataDescriptorSets() const { return _scene3DDescriptorSets; }

        inline size_t getCurrentFrame() const { return _currentFrame; }

    private:
        void createShadowPassResources();
        void destroyShadowPassResources();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();
        void destroyPipelines();

        void createShaderResources();
        void destroyShaderResources();
        // NOTE: Currently creating only once the "common" shader resources
        void createCommonShaderResources();
        void destroyCommonShaderResources();

        const CommandBuffer& recordCommandBuffer();
        void handleWindowResize();
    };
}
