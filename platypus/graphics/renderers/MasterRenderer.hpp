#pragma once

#include "platypus/graphics/CommandBuffer.hpp"
#include "platypus/graphics/Swapchain.hpp"
#include "platypus/graphics/Descriptors.hpp"
#include "platypus/graphics/Framebuffer.hpp"
#include "platypus/graphics/RenderPass.hpp"
#include "platypus/graphics/RenderPassInstance.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/assets/Material.hpp"
#include "GUIRenderer.hpp"
#include "Renderer3D.hpp"
#include "PostProcessingRenderer.hpp"
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

        // NOTE: Danger if adding anything after this, since the layout has to be taken into account!
        alignas(16) float time = 0.0f;
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
        std::unique_ptr<PostProcessingRenderer> _pPostProcessingRenderer;
        std::unique_ptr<GUIRenderer> _pGUIRenderer;

        RenderPass _shadowPass;
        RenderPass _opaquePass;
        RenderPass _transparentPass;
        // NOTE: Switched using single framebuffer and textures for testing offscreen rendering..
        //  -> This should be fine since these are produced and consumed by GPU and CPU doesn't
        //  touch these + the mem barrier in render commands
        Texture* _pDepthAttachment = nullptr;
        Texture* _pColorAttachment = nullptr;
        ImageFormat _offscreenColorFormat = ImageFormat::NONE;
        ImageFormat _offscreenDepthFormat = ImageFormat::NONE;
        TextureSampler _offscreenTextureSampler;
        Framebuffer* _pOpaqueFramebuffer = nullptr;
        Framebuffer* _pTransparentFramebuffer = nullptr;

        uint32_t _shadowmapWidth = 2048;
        RenderPassInstance _shadowPassInstance;
        DescriptorSetLayout _shadowmapDescriptorSetLayout;

        size_t _currentFrame = 0;

    public:
        // NOTE: CommandPool and Device must exist when creating this
        MasterRenderer(Swapchain& swapchain, ImageFormat shadowmapDepthFormat);
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
            bool instanced,
            bool skinned,
            bool shadowPipeline,
            std::vector<DescriptorSetLayout>& outDescriptorSetLayouts
        ) const;

        void setPostProcessingProperties(float bloomIntensity);

        inline const RenderPass& getShadowPass() const { return _shadowPass; }

        inline const Swapchain& getSwapchain() const { return _swapchainRef; }
        inline DescriptorPool& getDescriptorPool() { return _descriptorPool; }

        inline const DescriptorSetLayout& getScene3DDataDescriptorSetLayout() const { return _scene3DDataDescriptorSetLayout; }
        inline const DescriptorSetLayout& getShadowmapDescriptorSetLayout() const { return _shadowmapDescriptorSetLayout; }

        inline const std::vector<DescriptorSet>& getScene3DDataDescriptorSets() const { return _scene3DDescriptorSets; }

        inline RenderPassInstance* getShadowPassInstance() { return &_shadowPassInstance; }
        inline Framebuffer* getOpaqueFramebuffer() { return _pOpaqueFramebuffer; }
        inline Framebuffer* getTransparentFramebuffer() { return _pTransparentFramebuffer; }

        inline size_t getCurrentFrame() const { return _currentFrame; }

    private:
        void createOffscreenPassResources();
        void destroyOffscreenPassResources();

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
