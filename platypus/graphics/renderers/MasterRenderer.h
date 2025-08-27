#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/ecs/components/Renderable.h"
#include "Renderer.h"
#include "GUIRenderer.h"
#include "SkinnedMeshRenderer.hpp"

#include <map>
#include <memory>


namespace platypus
{
    struct Scene3DData
    {
        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f viewMatrix = Matrix4f(1.0f);
        Vector4f cameraPosition = Vector4f(0, 0, 0, 1);
        Vector4f lightDirection = Vector4f(0, 0, 0, 0);
        Vector4f lightColor = Vector4f(1, 1, 1, 1);
        Vector4f ambientLightColor = Vector4f(0.1f, 0.1f, 0.1f, 1);
    };

    class MasterRenderer
    {
    private:
        Swapchain _swapchain;
        DescriptorPool _descriptorPool;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        // Shared descriptor sets among multiple renderers
        Scene3DData _scene3DData;
        std::vector<Buffer*> _scene3DDataUniformBuffers;
        DescriptorSetLayout _scene3DDataDescriptorSetLayout;
        std::vector<DescriptorSet> _scene3DDescriptorSets;

        std::unique_ptr<Renderer> _pStaticMeshRenderer;
        std::unique_ptr<Renderer> _pSkinnedMeshRenderer;
        // NOTE: GUIRenderer is atm a little special case and it doesn't inherit Renderer
        // *This was due to Renderer and Material rework
        std::unique_ptr<GUIRenderer> _pGUIRenderer;

        // Key is the required component mask for submitted components of the renderer
        std::map<uint64_t, Renderer*> _renderers;

    public:
        // NOTE: CommandPool and Device must exist when creating this
        MasterRenderer(const Window& window);
        ~MasterRenderer();
        void createPipelines();

        void cleanRenderers();
        void cleanUp();
        void submit(const Scene* pScene, const Entity& entity);
        void render(const Window& window);

        inline SkinnedMeshRenderer* getSkinnedMeshRenderer() { return (SkinnedMeshRenderer*)_pSkinnedMeshRenderer.get(); }
        inline const SkinnedMeshRenderer* getSkinnedMeshRenderer() const { return (SkinnedMeshRenderer*)_pSkinnedMeshRenderer.get(); }

        inline const Swapchain& getSwapchain() const { return _swapchain; }
        inline DescriptorPool& getDescriptorPool() { return _descriptorPool; }

        inline const DescriptorSetLayout& getScene3DDataDescriptorSetLayout() const { return _scene3DDataDescriptorSetLayout; }

    private:
        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();
        void destroyPipelines();

        // NOTE: Currently creating only once the "common" shader resources
        void createCommonShaderResources();

        // Creates secondary renderers' and Material assets' shader resources
        void createShaderResources();
        // Frees secondary renderers' and Material assets' shader resources
        void freeShaderResources();

        const CommandBuffer& recordCommandBuffer();
        void handleWindowResize();
    };
}
