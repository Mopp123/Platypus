#include "MasterRenderer.h"
#include "platypus/core/Application.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Component.h"
#include "StaticMeshRenderer.h"


namespace platypus
{
    MasterRenderer::MasterRenderer(const Window& window) :
        _swapchain(window),
        _descriptorPool(_swapchain),

        _scene3DDataDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { 0, ShaderDataType::Mat4 },
                        { 1, ShaderDataType::Mat4 },
                        { 2, ShaderDataType::Float4 },
                        { 3, ShaderDataType::Float4 },
                        { 4, ShaderDataType::Float4 },
                        { 5, ShaderDataType::Float4 }
                    }
                }
            }
        )
    {
        _pStaticMeshRenderer = std::make_unique<StaticMeshRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE | ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        _pSkinnedMeshRenderer = std::make_unique<SkinnedMeshRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE | ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
        );
        _pGUIRenderer = std::make_unique<GUIRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE | ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        _renderers[_pStaticMeshRenderer->getRequiredComponentsMask()] = _pStaticMeshRenderer.get();
        _renderers[_pSkinnedMeshRenderer->getRequiredComponentsMask()] = _pSkinnedMeshRenderer.get();

        allocCommandBuffers(_swapchain.getMaxFramesInFlight());

        createCommonShaderResources();
    }

    MasterRenderer::~MasterRenderer()
    {
        _descriptorPool.freeDescriptorSets(_scene3DDescriptorSets);

        for (Buffer* pBuffer : _scene3DDataUniformBuffers)
            delete pBuffer;
        _scene3DDataUniformBuffers.clear();
        _scene3DDataDescriptorSetLayout.destroy();
    }

    void MasterRenderer::cleanRenderers()
    {
        Device::wait_for_operations();
        for (auto& it : _renderers)
            it.second->freeBatches();

        _pGUIRenderer->freeBatches();

        freeShaderResources();

        // NOTE: This is confusing as fuck atm:
        //  -> need to create renderer specific descriptor
        //  sets and uniform buffers here so they are ready for next scene
        //      -> Current freeDescriptorSets() clears the materials'
        //      descriptor sets and uniform buffers...
        for (auto& it : _renderers)
            it.second->createDescriptorSets();
    }

    void MasterRenderer::cleanUp()
    {
        cleanRenderers();
        destroyPipelines();
        freeCommandBuffers();
    }

    void MasterRenderer::submit(const Scene* pScene, const Entity& entity)
    {
        for (auto& it : _renderers)
        {
            if ((entity.componentMask & it.second->getRequiredComponentsMask()) == it.second->getRequiredComponentsMask())
                it.second->submit(pScene, entity.id);
        }
        if ((entity.componentMask & _pGUIRenderer->getRequiredComponentsMask()) == _pGUIRenderer->getRequiredComponentsMask())
            _pGUIRenderer->submit(pScene, entity.id);
    }

    void MasterRenderer::render(const Window& window)
    {
        SwapchainResult result = _swapchain.acquireImage();
        if (result == SwapchainResult::ERROR)
        {
            Debug::log(
                "@MasterRenderer::render "
                "Failed to acquire swapchain image!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        else if (result == SwapchainResult::RESIZE_REQUIRED)
        {
            handleWindowResize();
        }
        else
        {
            const CommandBuffer& cmdBuf = recordCommandBuffer();
            Device::submit_primary_command_buffer(
                _swapchain,
                cmdBuf,
                _swapchain.getCurrentFrame()
            );

            // present may also tell us to recreate swapchain!
            if (_swapchain.present() == SwapchainResult::RESIZE_REQUIRED || window.resized())
                handleWindowResize();
        }
    }

    void MasterRenderer::allocCommandBuffers(uint32_t count)
    {
        _primaryCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            count,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        );
        for (const auto& renderer : _renderers)
            renderer.second->allocCommandBuffers(count);

        _pGUIRenderer->allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        for (auto& it : _renderers)
            it.second->freeCommandBuffers();

        _pGUIRenderer->freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();
        _primaryCommandBuffers.clear();
    }

    void MasterRenderer::createPipelines()
    {
        const Extent2D swapchainExtent = _swapchain.getExtent();

        _pGUIRenderer->createPipeline(
            _swapchain.getRenderPass(),
            swapchainExtent.width,
            swapchainExtent.height
        );

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->recreateExistingPipeline();
    }

    void MasterRenderer::destroyPipelines()
    {
        _pGUIRenderer->destroyPipeline();

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->destroyPipeline();
    }

    void MasterRenderer::createCommonShaderResources()
    {
        // Create common uniform buffers and descriptor sets
        Scene3DData scene3DData;
        for (int i = 0; i < _swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pScene3DDataUniformBuffer = new Buffer(
                &scene3DData,
                sizeof(scene3DData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _scene3DDataUniformBuffers.push_back(pScene3DDataUniformBuffer);

            _scene3DDescriptorSets.push_back(
                _descriptorPool.createDescriptorSet(
                    &_scene3DDataDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pScene3DDataUniformBuffer } }
                )
            );
        }
    }

    void MasterRenderer::createShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->createShaderResources();

        for (auto& it : _renderers)
            it.second->createDescriptorSets();
    }

    void MasterRenderer::freeShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->freeShaderResources();

        for (auto& it : _renderers)
            it.second->freeDescriptorSets();
    }

    const CommandBuffer& MasterRenderer::recordCommandBuffer()
    {
        size_t frame = _swapchain.getCurrentFrame();

        if (frame >= _primaryCommandBuffers.size())
        {
            Debug::log(
                "@MasterRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(frame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_primaryCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _primaryCommandBuffers[frame];

        currentCommandBuffer.begin(_swapchain.getRenderPass());

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        Scene* pScene = sceneManager.accessCurrentScene();

        render::begin_render_pass(
            currentCommandBuffer,
            _swapchain,
            pScene->environmentProperties.clearColor,
            true
        );

        // NOTE: We create new copies of secondary command buffers here
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;

        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f orthographicProjectionMatrix = Matrix4f(1.0f);
        Matrix4f viewMatrix = Matrix4f(1.0f);

        Camera* pCamera = (Camera*)pScene->getComponent(ComponentType::COMPONENT_TYPE_CAMERA);
        Transform* pCameraTransform = (Transform*)pScene->getComponent(ComponentType::COMPONENT_TYPE_TRANSFORM);
        Vector4f cameraPosition;
        if (pCamera)
        {
            perspectiveProjectionMatrix = pCamera->perspectiveProjectionMatrix;
            orthographicProjectionMatrix = pCamera->orthographicProjectionMatrix;
        }
        _scene3DData.perspectiveProjectionMatrix = perspectiveProjectionMatrix;

        if (pCameraTransform)
        {
            const Matrix4f cameraTransformationMatrix = pCameraTransform->globalMatrix;
            cameraPosition.x = cameraTransformationMatrix[0 + 3 * 4];
            cameraPosition.y = cameraTransformationMatrix[1 + 3 * 4];
            cameraPosition.z = cameraTransformationMatrix[2 + 3 * 4];
            viewMatrix = cameraTransformationMatrix.inverse();
        }
        _scene3DData.cameraPosition = cameraPosition;
        _scene3DData.viewMatrix = viewMatrix;

        const DirectionalLight* pDirectionalLight = (const DirectionalLight*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT,
            false
        );
        if (!pDirectionalLight)
        {
            // TODO: Normalize
            _scene3DData.lightDirection = { 0.75f, -1.5f, -1.0f, 0.0f };
            _scene3DData.lightColor = { 0, 0, 0, 1 };
        }
        else
        {
            _scene3DData.lightDirection = {
                pDirectionalLight->direction.x,
                pDirectionalLight->direction.y,
                pDirectionalLight->direction.z,
                0.0f
            };
            _scene3DData.lightColor = {
                pDirectionalLight->color.r,
                pDirectionalLight->color.g,
                pDirectionalLight->color.b,
                1.0f
            };
        }

        const Vector3f sceneAmbientLight = pScene->environmentProperties.ambientColor;
        _scene3DData.ambientLightColor = {
            sceneAmbientLight.r,
            sceneAmbientLight.g,
            sceneAmbientLight.b,
            1.0f
        };

        _scene3DDataUniformBuffers[frame]->updateDeviceAndHost(
            &_scene3DData,
            sizeof(Scene3DData),
            0
        );

        const Extent2D swapchainExtent = _swapchain.getExtent();

        for (auto& it : _renderers)
        {
            secondaryCommandBuffers.push_back(
                it.second->recordCommandBuffer(
                    _swapchain.getRenderPass(),
                    swapchainExtent.width,
                    swapchainExtent.height,
                    _scene3DDescriptorSets[frame],
                    frame // NOTE: no idea should this be the "frame index" or "image index"
                )
            );
        }

        secondaryCommandBuffers.push_back(
            _pGUIRenderer->recordCommandBuffer(
                _swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                orthographicProjectionMatrix,
                frame
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    void MasterRenderer::handleWindowResize()
    {
        Application* pApp = Application::get_instance();
        Device::wait_for_operations();

        Window& window = pApp->getWindow();
        if (!window.isMinimized())
        {
            Device::handle_window_resize();
            _swapchain.recreate(window);
            freeShaderResources();
            destroyPipelines();
            freeCommandBuffers();
            allocCommandBuffers(_swapchain.getMaxFramesInFlight());
            createPipelines();
            createShaderResources();
            window.resetResized();
        }
        else
        {
            pApp->getInputManager().waitEvents();
        }
    }
}
