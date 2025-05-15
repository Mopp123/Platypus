#include "MasterRenderer.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Component.h"
#include "StaticMeshRenderer.h"
#include "GUIRenderer.h"


namespace platypus
{
    MasterRenderer::MasterRenderer(
        const Window& window
    ) :
        _swapchain(window),
        _descriptorPool(_swapchain),
        _cameraDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { 1, ShaderDataType::Float4 },
                        { 2, ShaderDataType::Mat4 }
                    }
                }
            }
        ),
        _dirLightDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    {
                        { 3, ShaderDataType::Float4 },
                        { 4, ShaderDataType::Float4 }
                    }
                }
            }
        )
    {
        // Create common uniform buffers and descriptor sets
        CameraUniformBufferData cameraUniformBufferData;
        for (int i = 0; i < _swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pCameraUniformBuffer = new Buffer(
                _commandPool,
                &cameraUniformBufferData,
                sizeof(CameraUniformBufferData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _cameraUniformBuffer.push_back(pCameraUniformBuffer);

            _cameraDescriptorSets.push_back(
                _descriptorPool.createDescriptorSet(
                    &_cameraDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pCameraUniformBuffer } }
                )
            );
        }

        DirLightUniformBufferData dirLightUniformBufferData;
        for (int i = 0; i < _swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pDirLightUniformBuffer = new Buffer(
                _commandPool,
                &dirLightUniformBufferData,
                sizeof(DirLightUniformBufferData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _dirLightUniformBuffer.push_back(pDirLightUniformBuffer);

            _dirLightDescriptorSets.push_back(
                _descriptorPool.createDescriptorSet(
                    &_dirLightDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pDirLightUniformBuffer } }
                )
            );
        }


        _pStaticMeshRenderer = std::make_unique<StaticMeshRenderer>(
            *this,
            _commandPool,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE | ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        _pGUIRenderer = std::make_unique<GUIRenderer>(
            *this,
            _commandPool,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE | ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        _renderers[_pStaticMeshRenderer->getRequiredComponentsMask()] = _pStaticMeshRenderer.get();
        _renderers[_pGUIRenderer->getRequiredComponentsMask()] = _pGUIRenderer.get();

        allocCommandBuffers(_swapchain.getMaxFramesInFlight());
        createPipelines();
    }

    MasterRenderer::~MasterRenderer()
    {
        for (Buffer* pBuffer : _cameraUniformBuffer)
            delete pBuffer;
        _cameraUniformBuffer.clear();
        _cameraDescriptorSetLayout.destroy();

        for (Buffer* pBuffer : _dirLightUniformBuffer)
            delete pBuffer;
        _dirLightUniformBuffer.clear();
        _dirLightDescriptorSetLayout.destroy();
    }

    void MasterRenderer::cleanRenderers()
    {
        Context::get_instance()->waitForOperations();
        for (auto& it : _renderers)
            it.second->freeBatches();

        freeDescriptorSets();
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
            Context::get_instance()->submitPrimaryCommandBuffer(
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
        _primaryCommandBuffers = _commandPool.allocCommandBuffers(
            count,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        );
        for (const auto& renderer : _renderers)
            renderer.second->allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        for (auto& it : _renderers)
            it.second->freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();
        _primaryCommandBuffers.clear();
    }

    void MasterRenderer::createPipelines()
    {
        const Extent2D swapchainExtent = _swapchain.getExtent();

        for (auto& it : _renderers)
        {
            it.second->createPipeline(
                _swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                _cameraDescriptorSetLayout,
                _dirLightDescriptorSetLayout
            );
        }
    }

    void MasterRenderer::destroyPipelines()
    {
        for (auto& it : _renderers)
            it.second->destroyPipeline();
    }

    void MasterRenderer::freeDescriptorSets()
    {
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

        // NOTE: We create new copies of secondary command buffers here!
        // Fucking stupid, since the actual command buffers we are using/refering to lives inside the
        // renderer producing the secondary command buffer.
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
        if (pCameraTransform)
        {
            const Matrix4f cameraTransformationMatrix = pCameraTransform->globalMatrix;
            cameraPosition.x = cameraTransformationMatrix[0 + 3 * 4];
            cameraPosition.y = cameraTransformationMatrix[1 + 3 * 4];
            cameraPosition.z = cameraTransformationMatrix[2 + 3 * 4];
            viewMatrix = cameraTransformationMatrix.inverse();
        }

        CameraUniformBufferData cameraUniformBufferData = { cameraPosition, viewMatrix };
        _cameraUniformBuffer[frame]->updateDeviceAndHost(
            &cameraUniformBufferData,
            sizeof(CameraUniformBufferData),
            0
        );

        const DirectionalLight* pDirectionalLight = (const DirectionalLight*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT,
            false
        );
        if (!pDirectionalLight)
        {
            _useDirLightData = { { 0, 0, 0, 1.0f }, { 0, 0, 0, 1.0f } };
        }
        else
        {
            _useDirLightData.direction = {
                pDirectionalLight->direction.x,
                pDirectionalLight->direction.y,
                pDirectionalLight->direction.z,
                1.0f
            };
            _useDirLightData.color = {
                pDirectionalLight->color.r,
                pDirectionalLight->color.g,
                pDirectionalLight->color.b,
                1.0f
            };
        }
        _dirLightUniformBuffer[frame]->updateDeviceAndHost(
            &_useDirLightData,
            sizeof(DirLightUniformBufferData),
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
                    perspectiveProjectionMatrix,
                    orthographicProjectionMatrix,
                    _cameraDescriptorSets[frame],
                    _dirLightDescriptorSets[frame],
                    frame // NOTE: no idea should this be the "frame index" or "image index"
                )
            );
        }

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    void MasterRenderer::handleWindowResize()
    {
        Context* pContext = Context::get_instance();
        Application* pApp = Application::get_instance();
        pContext->waitForOperations();

        Window& window = pApp->getWindow();
        if (!window.isMinimized())
        {
            pContext->handleWindowResize();
            _swapchain.recreate(window);
            freeDescriptorSets();
            destroyPipelines();
            freeCommandBuffers();
            allocCommandBuffers(_swapchain.getMaxFramesInFlight()); // Updated to test this...
            createPipelines();
            window.resetResized();
        }
        else
        {
            pApp->getInputManager().waitEvents();
        }
    }
}
