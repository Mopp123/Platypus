#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/ecs/components/Renderable.h"
#include "Renderer.h"

#include <map>
#include <memory>


namespace platypus
{
    struct CameraUniformBufferData
    {
        Vector4f position;
        Matrix4f viewMatrix = Matrix4f(1.0f);
    };


    struct DirLightUniformBufferData
    {
        Vector4f direction = Vector4f(0, 0, 0, 1);
        Vector4f color = Vector4f(1, 1, 1, 1);
    };


    class MasterRenderer
    {
    private:
        Swapchain _swapchain;
        CommandPool _commandPool;
        DescriptorPool _descriptorPool;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        // Shared descriptor sets among multiple renderers
        std::vector<Buffer*> _cameraUniformBuffer;
        DescriptorSetLayout _cameraDescriptorSetLayout;
        std::vector<DescriptorSet> _cameraDescriptorSets;

        DirLightUniformBufferData _useDirLightData;
        std::vector<Buffer*> _dirLightUniformBuffer;
        DescriptorSetLayout _dirLightDescriptorSetLayout;
        std::vector<DescriptorSet> _dirLightDescriptorSets;


        std::unique_ptr<Renderer> _pStaticMeshRenderer;
        std::unique_ptr<Renderer> _pGUIRenderer;

        // Key is the required component mask for submitted components of the renderer
        std::map<uint64_t, Renderer*> _renderers;

    public:
        MasterRenderer(const Window& window);
        ~MasterRenderer();
        void createPipelines();

        void cleanRenderers();
        void cleanUp();
        void submit(const Scene* pScene, const Entity& entity);
        void render(const Window& window);

        inline const Swapchain& getSwapchain() const { return _swapchain; }
        inline CommandPool& getCommandPool() { return _commandPool; }
        inline DescriptorPool& getDescriptorPool() { return _descriptorPool; }

        inline const DescriptorSetLayout& getCameraDescriptorSetLayout() const { return _cameraDescriptorSetLayout; }
        inline const DescriptorSetLayout& getDirectionalLightDescriptorSetLayout() const { return _dirLightDescriptorSetLayout; }

    private:
        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();
        void destroyPipelines();
        void freeDescriptorSets();
        void createDescriptorSets();
        const CommandBuffer& recordCommandBuffer();
        void handleWindowResize();
    };
}
